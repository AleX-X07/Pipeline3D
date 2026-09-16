[[vk::binding(0, 0)]] public Sampler2D   textures[];
[[vk::binding(1, 0)]] public RWTexture2D storageTextures[];

static const float kPI          = 3.14159265359f;
static const float kONE_OVER_PI = 0.31830988618f;

public enum ELightType
{
    Directional,
    Point,
    Spot
};

public struct Light
{
    float4x4           viewProj;
    public ELightType  type;
    public  float3     position;
    private float _0;
    public  float3     direction;
    private float _1;
    public  float3     color;
    private float _2;
    public  float      intensity;
    public  float      spotInnerAngle;
    public  float      spotOuterAngle;
    public  float      radius;
    public  uint32_t   shadowmapId;
};

struct FrameUniforms
{
	float4x4    viewProj;
	float4x4    viewProjInverse;
	float3      eye;
	uint        frameIndex;
	uint64_t    __materials;
	uint32_t    environmentDiffuseId;
	uint32_t    environmentSpecularId;
	float       environmentIntensity;
};

struct PushConstants
{
    FrameUniforms* frameUniforms;
    Light*         lights;
    uint           numLights;
    uint           rt0;
    uint           rt1;
    uint           zbuffer;
    uint           output;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

public struct ShadeMaterial
{
    public float3 albedo;
    public float  metalness;
    public float  roughness;
};

public float3 AdjustPointLightForRadius(float3 R, float3 L_unnorm, float radius)
{
    float3 centerToRay = dot(L_unnorm, R) * R - L_unnorm;
    float len = length(centerToRay);
    float3 closestPoint = L_unnorm + centerToRay * saturate(radius / max(1e-6f, len));
    return normalize(closestPoint);
}

public float D_GGX_TrowbridgeReitz(float NoH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH2 = NoH * NoH;
    float denom = NoH2 * (a2 - 1.0f) + 1.0f;
    return a2 / (kPI * max(1e-7f, denom * denom));
}

public float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

public float G_Smith(float NoV, float NoL, float roughness)
{
    float ggx1 = G_SchlickGGX(NoV, roughness);
    float ggx2 = G_SchlickGGX(NoL, roughness);
    return ggx1 * ggx2;
}

public float3 F_Schlick(float VoH, float3 f0)
{
    float f = pow(1.0f - VoH, 5.0f);
    return f0 + (1.0f - f0) * f;
}

public float3 EvaluateSpecularBRDF(ShadeMaterial material, float NoL, float NoV, float NoH, float VoH)
{
    float3 f0 = lerp(0.04f.xxx, material.albedo, material.metalness);

    // Cook-Torrance BRDF components
    float  D = D_GGX_TrowbridgeReitz(NoH, material.roughness);
    float  G = G_Smith(NoV, NoL, material.roughness);
    float3 F = F_Schlick(VoH, f0);

    return (D * G * F) / max(1e-3f, 4.0f * NoV * NoL);
}

public float3 EvaluateDiffuseBRDF(ShadeMaterial material, float NoL, float NoV, float NoH, float VoH)
{
    float3 f0 = lerp(0.04f.xxx, material.albedo, material.metalness);
    float3 F  = F_Schlick(VoH, f0);

    float3 kD = (1.0f.xxx - F) * (1.0f - material.metalness);
    return kD * material.albedo * kONE_OVER_PI;
}

public float ComputeShadow(uint index, float3 wsPos)
{
    Light light = pc.lights[index];

    if (light.shadowmapId == uint32_t(-1))
        return 1.0f;

    float4 lightClip = mul(light.viewProj, float4(wsPos, 1.0f));

    if (lightClip.w <= 0.0f)
        return 1.0f;

    float3 lightNDC = lightClip.xyz / lightClip.w;
    if (any(abs(lightNDC.xy) > 1.0f) || lightNDC.z < 0.0f || lightNDC.z > 1.0f)
        return 1.0f;

    float2 uv = lightNDC.xy * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;

    float currentDepth = lightNDC.z;

    const float kBias = 0.0015f;

    uint texWidth, texHeight;
    textures[light.shadowmapId].GetDimensions(texWidth, texHeight);
    float2 texelSize = 1.0f.xx / float2(texWidth, texHeight);

    float shadow = 0.0f;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float sampleDepth = textures[light.shadowmapId]
                .SampleLevel(uv + float2(x, y) * texelSize, 0).r;
            shadow += (currentDepth - kBias > sampleDepth) ? 0.0f : 1.0f;
        }
    }

    return shadow / 9.0f;
}

float3 Shade(uint2 pixelId, ShadeMaterial material, float3 pos, float3 N, float3 V, uint index)
{
    var light = pc.lights[index];

    float3 ld = -normalize(light.direction);

    float3 L_unnorm = light.position - pos;
    float  dist     = length(L_unnorm);
    float3 L        = light.type == ELightType::Directional ? ld : L_unnorm / dist;

    if (dot(N, L) < 0.0f)
        return 0.0f.xxx;

    // Closest point on sphere for specular
    float3 R        = reflect(-V, N);
    float3 L_spec   = light.type == ELightType::Directional
        ? L
        : AdjustPointLightForRadius(R, L_unnorm, light.radius);

    float3 H   = normalize(L_spec + V);
    float  NoL = max(0.0f, dot(N, L));
    float  NoL_spec = max(0.0f, dot(N, L_spec));
    float  NoV = max(0.0f, dot(N, V));
    float  NoH = max(0.0f, dot(N, H));
    float  VoH = max(0.0f, dot(V, H));

    float3 diffuse  = EvaluateDiffuseBRDF(material, NoL, NoV, NoH, VoH);
    float3 specular = EvaluateSpecularBRDF(material, NoL_spec, NoV, NoH, VoH);
    
    float distWithRadius = max(dist - light.radius, 1e-2f);

    float attenuation = light.type == ELightType::Directional
        ? 1.0f
        : 1.0f / (distWithRadius * distWithRadius);

    float shadow = ComputeShadow(index, pos);

    float spotAttenuation = light.type == ELightType::Spot
        ? 1.0f - smoothstep(light.spotInnerAngle, light.spotOuterAngle, dot(L, -ld))
        : 1.0f;

    float3 lightFactor = light.color * light.intensity * attenuation * spotAttenuation * shadow;
    return (NoL * diffuse + NoL_spec * specular) * lightFactor;
}

float3 SampleIBL(uint texId, float3 dir)
{
    if (texId == uint32_t(-1))
        return 0.0f.xxx;

    float phi   = atan2(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0f, 1.0f));

    float2 uv = float2(
        phi   * kONE_OVER_PI * 0.5f + 0.5f,
        theta * kONE_OVER_PI
    );

    return textures[texId].SampleLevel(uv, 0).rgb * pc.frameUniforms.environmentIntensity;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 threadId: SV_DispatchThreadID)
{
    RWTexture2D output = storageTextures[pc.output];

    uint outputWidth, outputHeight;
    output.GetDimensions(outputWidth, outputHeight);

    if (threadId.x >= outputWidth || threadId.y >= outputHeight)
        return;

    float4 rt0 = textures[pc.rt0].Load(int3(threadId.xy, 0));
    float4 rt1 = textures[pc.rt1].Load(int3(threadId.xy, 0));
    float  rtZ = textures[pc.zbuffer].Load(int3(threadId.xy, 0)).r;

    float2 ndc2d = float2(
        (float(threadId.x) / float(outputWidth)) * 2.0f - 1.0f,
        (float(threadId.y) / float(outputHeight)) * 2.0f - 1.0f
    );
    ndc2d.y = -ndc2d.y;

    float4 ndc = float4(ndc2d, rtZ, 1.0f);
    float4 ws = mul(pc.frameUniforms.viewProjInverse, ndc);
    ws /= ws.w;

    float3 V = normalize(pc.frameUniforms->eye - ws.xyz);

    if (rtZ == 1.0f)
    {
        output[threadId.xy] = float4(SampleIBL(pc.frameUniforms.environmentDiffuseId, -V), 1.0f);
        return;
    }

    ShadeMaterial material = ShadeMaterial(rt0.rgb, rt0.a, rt1.a);

    float3 N = normalize(rt1.xyz);
    float3 R = reflect(-V, N);

    float NoR = saturate(dot(N, R));

    float3 iblDiffuse  = SampleIBL(pc.frameUniforms.environmentDiffuseId, N) * material.albedo * (1.0f - material.metalness);

    float3 iblSpecular = 0.0f.xxx;
    {
        float3 f0 = lerp(0.04f.xxx, material.albedo, material.metalness);

        // Cook-Torrance BRDF components
        float3 F = F_Schlick(NoR, f0);

        iblSpecular = F * SampleIBL(pc.frameUniforms.environmentSpecularId, R);
    }

    float3 result = iblDiffuse + iblSpecular;
    for (uint i = 0; i < pc.numLights; ++i)
        result += Shade(threadId.xy, material, ws.xyz, N, V, i);

    // Basic Reinhard tonemapping
    result = result / (result + 1.0f.xxx);

    output[threadId.xy] = float4(result, 1.0f);
}

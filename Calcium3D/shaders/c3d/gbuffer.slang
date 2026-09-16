[[vk::binding(0, 0)]] public Sampler2D   textures[];
[[vk::binding(1, 0)]] public RWTexture2D storageTextures[];

public struct SubmeshData
{
    public float4x4 local;
    public float4x4 normal;
};

public struct MaterialValue
{
	float4 value;
	uint4  textureId;
};

public struct Material
{
	MaterialValue baseColor;
    MaterialValue orm;
    MaterialValue normal;
};

struct FrameUniforms
{
	float4x4    viewProj;
	float4x4    viewProjInverse;
	float3      eye;
	uint        frameIndex;
	Material*   materials;
	uint32_t    environmentId;
	float       environmentIntensity;
};

struct PushConstants
{
    FrameUniforms* frameUniforms;
    SubmeshData*   submeshData;
    float4x4       model;
    uint32_t       materialId;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

// ----------------------------------------------------------------------------

struct VSIn
{
    float3 position;
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 previousPosition;
};

struct VSOut
{
    float4 position : SV_Position;
    float3 worldPos;
    float3 normal;
    float2 uv;
    float3 tangent;
};

[shader("vertex")]
VSOut vs_main(VSIn input)
{
    float4 position     = float4(input.position, 1.0f);
    float4 normal       = float4(input.normal,   0.0f);
    float4 tangent      = float4(input.tangent,  0.0f);

    float4 P = mul(pc.submeshData.local,  position);
    float4 N = mul(pc.submeshData.normal, normal);
    float4 T = mul(pc.submeshData.normal, tangent);

    float4 wsPos = mul(pc.model, P);
    return VSOut(
        mul(pc.frameUniforms->viewProj, wsPos),
        wsPos.xyz,
        normalize(mul(pc.model, N).xyz),
        input.uv,
        normalize(mul(pc.model, T).xyz),
    );
};

// ----------------------------------------------------------------------------

struct FSOut
{
    float4 rt0;
    float4 rt1;
};

float3 GetBaseColor(uint materialId, float2 uv)
{
    float3 result = 1.0f.xxx;

    if (materialId != uint(-1))
    {
        result = pc.frameUniforms.materials[materialId].baseColor.value.rgb;

        uint texId = pc.frameUniforms.materials[materialId].baseColor.textureId.r;
        if (texId != uint(-1))
            result *= textures[texId].Sample(uv).rgb;
    }

    return pow(result, 2.2f.xxx);
}

float3 GetORM(uint materialId, float2 uv)
{
    float3 result = float3(1.0f, 0.0f, 0.25f);

    if (materialId != uint(-1))
    {
        uint texId = pc.frameUniforms.materials[materialId].orm.textureId.r;
        if (texId != uint(-1))
            result = textures[texId].Sample(uv).rgb;
        else
            result = pc.frameUniforms.materials[materialId].orm.value.rgb;

    }

    return result;
}

float3 GetNormal(uint materialId, float3 N, float3 T, float2 uv)
{
    float3 result = N;

    if (materialId != uint(-1))
    {
        uint texId = pc.frameUniforms.materials[materialId].normal.textureId.r;
        if (texId != uint(-1))
        {
            float3 texN = textures[texId].Sample(uv).rgb * 2.0f - 1.0f.xxx;
            result = mul(texN, float3x3(T, cross(N, T), N));
        }
    }

    return normalize(result);
}

[shader("fragment")]
FSOut fs_main(VSOut input)
{
    FSOut output;

    float3 baseColor = GetBaseColor(pc.materialId, input.uv);
    float3 orm       = GetORM(pc.materialId, input.uv);
    float  metalness = orm.z;
    float  roughness = orm.y;
    float3 normal    = GetNormal(pc.materialId, input.normal, input.tangent, input.uv);

    output.rt0 = float4(baseColor, metalness);
    output.rt1 = float4(normal, roughness);

    return output;
}

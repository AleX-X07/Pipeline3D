static const float2 VIRTUAL_RESOLUTION = float2(1920.0, 1080.0);

[[vk::binding(0, 0)]] public Sampler2D   textures[];
[[vk::binding(1, 0)]] public RWTexture2D storageTextures[];

enum EShape2DType
    : uint32_t
{
    Box,
    Circle
};

struct Shape
{
    EShape2DType type;
    uint32_t     image;
    uint32_t     imageIsMsdf;
    float        msdfScale;
    float4       transform[3];
    float4       color;
    float2       uvOffset;
    float2       uvScale;
};

struct PushConstants
{
    Shape* shapes;
    uint   shapeCount;
    uint   output;
};
[[vk::push_constant]] ConstantBuffer<PushConstants> pc;

float2 pixelToVirtual(uint2 pixel, uint2 outputSize)
{
    return (float2(pixel) + 0.5f) / float2(outputSize) * VIRTUAL_RESOLUTION;
}

float3x3 inverse3x3(float3x3 m)
{
    float det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
              - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
              + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    float inv = 1.0f / det;
    float3x3 r;
    r[0][0] =  (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * inv;
    r[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * inv;
    r[0][2] =  (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * inv;
    r[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * inv;
    r[1][1] =  (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * inv;
    r[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) * inv;
    r[2][0] =  (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * inv;
    r[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) * inv;
    r[2][2] =  (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * inv;
    return r;
}

float3x3 makeMatrix(float4 from[3])
{
    return float3x3(from[0].xyz, from[1].xyz, from[2].xyz);
}

float2 worldToLocal(float4 localToWorld[3], float2 worldPos)
{
    float3x3 worldToLocalMat = inverse3x3(makeMatrix(localToWorld));
    float3 local = mul(worldToLocalMat, float3(worldPos, 1.0));
    return local.xy;
}

float sdfBox(float2 p)
{
    float2 d = abs(p) - 0.5f;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0f);
}

float sdfCircle(float2 p)
{
    return length(p) - 0.5f;
}

float sdfToAlpha(float dist, float pixelSize)
{
    return saturate(-dist / pixelSize + 0.5f);
}

float localPixelSize(float4 localToWorld[3])
{
    float3x3 inv = inverse3x3(makeMatrix(localToWorld));
    float scaleX = length(float2(inv[0][0], inv[0][1]));
    float scaleY = length(float2(inv[1][0], inv[1][1]));
    return (scaleX + scaleY) * 0.5f;
}

float4 sampleShape(Shape shape, float2 worldPos)
{
    float2 localPos = worldToLocal(shape.transform, worldPos);

    float dist;
    if (shape.type == EShape2DType::Box)
        dist = sdfBox(localPos);
    else
        dist = sdfCircle(localPos);

    float pixelSize = localPixelSize(shape.transform);
    float alpha = sdfToAlpha(dist, pixelSize);

    if (alpha <= 0.0f)
        return 0.0f.xxxx;

    float4 color = shape.color;

    if (shape.image != uint32_t(-1))
    {
        uint tw, th;
        textures[shape.image].GetDimensions(tw, th);
        float2 textureSize = float2(tw, th);
        float2 halfTexel = 0.5f / textureSize;

        float2 uv0 = shape.uvOffset + halfTexel;
        float2 uv1 = shape.uvOffset + shape.uvScale - halfTexel;

        float2 uv = localPos + 0.5f;
        uv = uv * shape.uvScale + shape.uvOffset;
        uv = clamp(uv, uv0, uv1);

        float4 texSample = textures[shape.image].SampleLevel(uv, 0);

        if (shape.imageIsMsdf != 0)
        {
            float scale = 1.0f / pixelSize;
            float dist = scale * (median3(texSample.r, texSample.g, texSample.b) - 0.55f);
            float o = saturate(dist + 0.55f);
            color.a *= o;
        }
        else
            color *= texSample;
    }

    color.a *= alpha;
    return color;
}

float4 blendOver(float4 dst, float4 src)
{
    float a = src.a + dst.a * (1.0f - src.a);
    if (a < 1e-6f)
        return 0.0f.xxxx;

    float3 rgb = (src.rgb * src.a + dst.rgb * dst.a * (1.0f - src.a)) / a;
    return float4(rgb, a);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 threadId : SV_DispatchThreadID)
{
    RWTexture2D output = storageTextures[pc.output];

    uint outputWidth, outputHeight;
    output.GetDimensions(outputWidth, outputHeight);

    if (threadId.x >= outputWidth || threadId.y >= outputHeight)
        return;

    float2 worldPos = pixelToVirtual(threadId.xy, uint2(outputWidth, outputHeight));

    float4 color = output[threadId.xy];

    for (uint i = 0; i < pc.shapeCount; i++)
    {
        float4 shapeColor = sampleShape(pc.shapes[i], worldPos);
        color = blendOver(color, shapeColor);
    }

    output[threadId.xy] = color;
}

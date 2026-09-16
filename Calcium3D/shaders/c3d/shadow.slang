public struct SubmeshData
{
    public float4x4 local;
    public float4x4 normal;
};

struct PushConstants
{
    float4x4     lightViewProj;
    float4x4     model;
    SubmeshData* submeshData;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

struct VSIn
{
    float3 position;
};

struct VSOut
{
    float4 position : SV_Position;
};

[shader("vertex")]
VSOut vs_main(VSIn input)
{
    float4 position = float4(input.position, 1.0f);
    float4 P        = mul(pc.submeshData.local, position);
    float4 wsPos    = mul(pc.model, P);

    return VSOut(mul(pc.lightViewProj, wsPos));
}

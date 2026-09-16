struct BindlessResources
{
    Sampler2D textures[];
};
ParameterBlock<BindlessResources> bindlessResources;

struct PushConstants
{
    float2   scale;
    float2   translate;
    uint32_t textureId;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

struct VSIn
{
    float2 pos;
    float2 uv;
    float4 col; // R8G8B8A8_UNorm unpacked to float4 by input assembly
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv;
    float4 col;
};

[shader("vertex")]
VSOut vs_main(VSIn i)
{
    float4 pos = float4(i.pos * pc.scale + pc.translate, 0.0f, 1.0f);
    pos.y = -pos.y;
    return VSOut(pos, i.uv, i.col);
}

[shader("fragment")]
float4 fs_main(VSOut i) : SV_Target
{
    Sampler2D tex = bindlessResources.textures[pc.textureId];
    float4 result = i.col * tex.Sample(i.uv);
    return float4(pow(result.rgb, 2.2f), result.a);
}

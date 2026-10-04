[[vk::binding(0, 0)]] public Sampler2D   textures[];
[[vk::binding(1, 0)]] public RWTexture2D storageTextures[];

enum class EScreenSplitDirection
    : uint32_t
{
    Horizontal,
    Vertical
}

struct PushConstants
{
    float4                lineProperties;
    uint                  split0;
    uint                  split1;
    uint                  output;
    EScreenSplitDirection splitDirection;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 threadId: SV_DispatchThreadID)
{
    RWTexture2D output = storageTextures[pc.output];

    uint outputWidth, outputHeight;
    output.GetDimensions(outputWidth, outputHeight);

    if (threadId.x >= outputWidth || threadId.y >= outputHeight)
        return;

    uint2 coord = threadId.xy;
    float4 result = 0.0f.xxxx;

    uint2 targetSize = uint2(outputWidth, outputHeight);
    uint2 size0 = targetSize;
    if (pc.splitDirection == EScreenSplitDirection::Vertical)
        size0.x >>= 1;
    else
        size0.y >>= 1;
        
    bool isLine = false;

    uint targetImage = pc.split0;
    if (pc.splitDirection == EScreenSplitDirection::Vertical)
    {
        if (abs(int(coord.x) - int(size0.x)) < int(pc.lineProperties.w))
            isLine = true;

        if (coord.x >= size0.x)
        {
            targetImage = pc.split1;
            coord.x -= size0.x;
        }
    }
    else
    {
        if (abs(int(coord.y) - int(size0.y)) < int(pc.lineProperties.w))
            isLine = true;

        if (coord.y >= size0.y)
        {
            targetImage = pc.split1;
            coord.y -= size0.y;
        }
    }
    
    result = textures[targetImage].Load(int3(coord, 0));
    if (isLine)
        result = float4(pc.lineProperties.rgb, 1.0f);

    output[threadId.xy] = float4(result.rgb, 1.0f);
}

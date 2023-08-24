cbuffer CBuffer : register(b0)
{
    matrix proj;
    matrix view;
    matrix wolrd;
}

struct VertexIn
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

struct VertexOut
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

VertexOut vs_main(VertexIn i)
{

    VertexOut o = (VertexOut)0;

    float4 wPos =  mul(float4(i.pos, 1.0f), wolrd);
    wPos = mul(wPos, view);
    wPos = mul(wPos, proj);
    o.pos = wPos;
    o.col = i.col;

    return o;
}

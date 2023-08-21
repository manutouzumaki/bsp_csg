struct FragmentIn
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

float4 fs_main(FragmentIn i) : SV_TARGET
{
    return i.col;
}

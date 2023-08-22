#ifndef _TYPES_H_
#define _TYPES_H_

struct Win32Renderer
{
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* renderTargetView;
    ID3D11DepthStencilView* depthStencilView;

    ID3D11RasterizerState* wireFrameRasterizer;
    ID3D11RasterizerState* fillRasterizerCullBack;
    ID3D11RasterizerState* fillRasterizerCullFront;
    ID3D11RasterizerState* fillRasterizerCullNone;
    ID3D11DepthStencilState* depthStencilOn;
    ID3D11DepthStencilState* depthStencilOff;
    ID3D11BlendState* alphaBlendEnable;
    ID3D11BlendState* alphaBlendDisable;
};

struct Win32Shader
{
    ID3D11VertexShader *vertex;
    ID3D11PixelShader *fragment;
    ID3DBlob *vertexShaderCompiled;
    ID3DBlob *fragmentShaderCompiled;
};

struct Win32ConstBuffer
{
    ID3D11Buffer *buffer;
    u32 index;
};

struct Win32VertexBuffer
{
    ID3D11Buffer *GPUBuffer;
    u32 verticesCount;

    ID3D11InputLayout *layout;
};

struct Win32File
{
    void *data;
    size_t size;
};

struct Memory
{
    u8 *data;
    size_t size;
    size_t used;
};

struct Arena
{
    u8 *base;
    size_t size;
    size_t used;
};

struct Vertex
{
    Vec3 position;
    Vec4 color;
};

struct CBuffer
{
    Mat4 proj;
    Mat4 view;
    Mat4 wolrd;
};

struct Face
{
    char textureName[32];
    Vec4 u, v;
    Vec3 a, b, c;
    f32 textureRotation;
    f32 uScale;
    f32 vScale;
};

struct Brush
{
    Face faces[256];
    u32 facesCount;
};

struct Entity
{
    Brush brushes[256];
    u32 brushesCount;
};

struct Map
{
    Entity entities[32];
    u32 entitiesCount;
};

struct Plane
{
    Vec3 n;
    f32 d;
};

struct GameState
{
    Map map;
};

#endif

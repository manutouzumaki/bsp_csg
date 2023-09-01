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
    Vec2 uv;
};

struct CBuffer
{
    Mat4 proj;
    Mat4 view;
    Mat4 wolrd;
};

struct MapFace
{
    char textureName[32];
    Vec4 u, v;
    Vec3 a, b, c;
    f32 textureRotation;
    f32 uScale;
    f32 vScale;
};

struct MapBrush
{
    MapFace *faces;
    u32 facesCount;
};

struct MapEntity
{
    MapBrush *brushes;
    u32 brushesCount;
};

struct Map
{
    MapEntity *entities;
    u32 entitiesCount;
};

struct Plane
{
    Vec3 n;
    f32 d;
};

struct PolygonData
{
    std::vector<Vertex> vertices; 
    Plane plane;
};

enum ClassifyPolygon
{
    POLYGON_COPLANAR_WITH_PLANE,
    POLYGON_IN_FRONT_OF_PLANE,
    POLYGON_BEHIND_PLANE,
    POLYGON_STRADDLING_PLANE
};

enum ClassifyPoint
{
    POINT_IN_FRONT_OF_PLANE,
    POINT_BEHIND_PLANE,
    POINT_ON_PLANE
};

enum BSPState
{
    BSP_ROOT,
    BSP_FRONT,
    BSP_BACK
};

struct GameState
{
    u32 pad;
};

#endif

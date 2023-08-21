#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <stdio.h>
#include <stdint.h>

#include "defines.h"
#include "math.h"
#include "types.h"

static bool gRunnig;

#include "arena.cpp"
#include "win32.cpp"

static void ParseFace(char **current, Face *face, char *end)
{
    sscanf(*current, "( %f %f %f ) ( %f %f %f ) ( %f %f %f ) %s [ %f %f %f %f ] [ %f %f %f %f ] %f %f %f",
           &face->a.x, &face->a.y, &face->a.z,
           &face->b.x, &face->b.y, &face->b.z,
           &face->c.x, &face->c.y, &face->c.z,
           face->textureName, 
           &face->u.x, &face->u.y, &face->u.z, &face->u.w,
           &face->v.x, &face->v.y, &face->v.z, &face->v.w,
           &face->textureRotation,
           &face->uScale,
           &face->vScale);

    while(**current != '\n') (*current)++;
}

static void ParseBrush(char **current, Brush *brush, char *end)
{
    while(*current != end)
    {
        if(**current == '(')
        {
            Face *face = brush->faces + brush->facesCount++;
            ParseFace(current, face, end);
        }

        (*current)++;
        if(**current == '}')
        {
            return;
        }
    }
}

static void ParseEntity(char **current, Entity *entity, char *end)
{
    while(*current != end)
    {
        if(**current == '{')
        {
           Brush *brush = entity->brushes + entity->brushesCount++; 
            (*current)++;
            ParseBrush(current, brush, end);
        }

        (*current)++;
        if(**current == '}')
        {
            return;
        }
    }
}

static void ParseMapFile(Win32File *file, GameState *gameState)
{
    Map *map = &gameState->map;
    char *start = (char *)file->data;
    char *end = (char *)file->data + file->size;
    char *current = start;

    while(current != end)
    {
        if(*current == '{')
        {
            Entity *entity = map->entities + map->entitiesCount++;
            current++;
            ParseEntity(&current, entity, end);
        }

        current++;
        if(*current == '}')
        {
            return;
        }
    }
}

bool GetIntersection(Vec3 n1, Vec3 n2, Vec3 n3, f32 d1, f32 d2, f32 d3, Vertex *vertex)
{
    f32 denom = Vec3Dot(n1, Vec3Cross(n2, n3));
    if(denom <= VEC_EPSILON && denom >= -VEC_EPSILON)
    {
        return false;
    }
    Vec3 pos = (-d1 * Vec3Cross(n2, n3) -d2 * Vec3Cross(n3, n1) -d3 * Vec3Cross(n1, n2)) / denom;
    Vec4 col = {0, 1, 0, 1};
    *vertex = {pos, col};
    return true;
}

Plane GetPlaneFromThreePoints(Vec3 a, Vec3 b, Vec3 c)
{
    Plane plane;
    Vec3 ab = b - a;
    Vec3 ac = c - a;
    Vec3 n = Vec3Normalized(Vec3Cross(ab, ac));
    f32 d = -n.x*a.x -n.y*a.y -n.z*a.z;
    return {n, d};
}

i32 main(void)
{
    HWND window = Win32CreateWindow("BSP_CSG_TEST", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    Win32Renderer renderer = Win32InitD3D11(window, WINDOW_WIDTH, WINDOW_HEIGHT);

    Memory memory = MemoryInit(GIGABYTES(3));

    // the game state should be the first thing in memory
    ASSERT(sizeof(GameState) <= memory.size);
    GameState *gameState = (GameState *)memory.data;
    memory.used += sizeof(GameState);

    Arena resourcesArena = ArenaCreate(&memory, MEGABYTES(25));

    Win32Shader shader = Win32LoadShader(&renderer, &memory, &resourcesArena,
                                         "../src/vertex.hlsl", "../src/fragment.hlsl");

    renderer.deviceContext->VSSetShader(shader.vertex, 0, 0);
    renderer.deviceContext->PSSetShader(shader.fragment, 0, 0);

    // create input layout. 
    ID3D11InputLayout *layout = 0;
    D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
         0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
         0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    int totalLayoutElements = ARRAY_LENGTH(inputLayoutDesc);
    renderer.device->CreateInputLayout(inputLayoutDesc,
        totalLayoutElements,
        shader.vertexShaderCompiled->GetBufferPointer(),
        shader.vertexShaderCompiled->GetBufferSize(),
        &layout);

    // render a rectangle test
    Vertex vertices[3] = {
        // top triangle
        {{-0.5f,  0.5f, 0}, {1, 0, 0, 1}},
        {{ 0.5f,  0.5f, 0}, {0, 1, 0, 1}},
        {{-0.5f, -0.5f, 0}, {0, 0, 1, 1}},
    };
    Win32VertexBuffer vertexBuffer = Win32LoadVertexBuffer(&renderer, vertices, ARRAY_LENGTH(vertices), layout);

    // create the const buffer data to be pass to the gpu
    Vec3 pos = {0, 0, -5};
    Vec3 tar = {0, 0,  0};
    Vec3 up  = {0, 1,  0};
    CBuffer cbuffer = {};
    cbuffer.view = Mat4LookAt(pos, tar, up);
    cbuffer.proj = Mat4Perspective(60, (f32)WINDOW_WIDTH/(f32)WINDOW_HEIGHT, 0.01f, 100.0f);
    cbuffer.wolrd = Mat4Identity();
    Win32ConstBuffer constBuffer = Win32LoadConstBuffer(&renderer, (void *)&cbuffer, sizeof(cbuffer), 0);

    // TODO: try to parse the valve's .map file
    Win32File mapFile = Win32ReadFile("../src/test.map", &resourcesArena);
    ParseMapFile(&mapFile, gameState);

    Map *map = &gameState->map;
    Brush *brush = &map->entities[0].brushes[0];
    Face *faces = brush->faces;
    u32 facesCount = brush->facesCount;

    Vertex convexPolygon[256];
    i32 vertexCount = 0;
    for(i32 i = 0; i < facesCount; ++i) {
        for(i32 j = 0;j < facesCount; ++j) {
            for(i32 k = 0; k < facesCount; ++k) {

                if(i != j && i != k && j != k)
                {
                    Plane a = GetPlaneFromThreePoints(faces[i].a, faces[i].b, faces[i].c); 
                    Plane b = GetPlaneFromThreePoints(faces[j].a, faces[j].b, faces[j].c); 
                    Plane c = GetPlaneFromThreePoints(faces[k].a, faces[k].b, faces[k].c); 
                    Vertex vertex = {};
                    if(GetIntersection(a.n, b.n, c.n, a.d, b.d, c.d, &vertex))
                    {
                        convexPolygon[vertexCount++] = vertex;
                    }
                }

            }
        }
    }

    i32 StopHere = 0;




    gRunnig = true;
    while(gRunnig)
    {
        Win32FlushEvents(window);

        static f32 angle = 0;
        Mat4 rotZ = Mat4RotateZ(angle);
        Mat4 rotX = Mat4RotateX(angle);
        Mat4 trans = Mat4Translate(sinf(angle), 0, 0);
        cbuffer.wolrd = trans * rotZ * rotX;
        angle += 0.016f;
        Win32UpdateConstBuffer(&renderer, &constBuffer, (void *)&cbuffer);

        f32 clearColor[] = { 0.2, 0.2, 0.2, 1 };
        renderer.deviceContext->ClearRenderTargetView(
                renderer.renderTargetView, clearColor); 
        renderer.deviceContext->ClearDepthStencilView(
                renderer.depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Here goes the rendering code
        u32 stride = sizeof(Vertex);
        u32 offset = 0;
        renderer.deviceContext->IASetInputLayout(vertexBuffer.layout);
        renderer.deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer.GPUBuffer, &stride, &offset);
        renderer.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        renderer.deviceContext->Draw(vertexBuffer.verticesCount, 0);

        renderer.swapChain->Present(1, 0);

    }

    Win32UnloadConstBuffer(&constBuffer);

    Win32UnloadVertexBuffer(&vertexBuffer);
    layout->Release();

    Win32UnloadShader(&shader);

    MemoryShutdown(&memory);

    Win32ShutdownD3D11(&renderer);

    return 0;
}

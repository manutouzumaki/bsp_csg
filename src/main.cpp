#include <windows.h>
#include <windowsX.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <vector>
#include <stack>

#include "defines.h"
#include "math.h"
#include "types.h"
#include "input.h"
#include "csg.h"
#include "map.h"
#include "bsp.h"
#include "game.h"

static bool gRunnig;
static InputState gCurrentInput;
static InputState gLastInput;
static i32 gWindowX;
static i32 gWindowY;
static f32 gMapScale = 128.0f;

#include "arena.cpp"
#include "win32.cpp"
#include "input.cpp"
#include "csg.cpp"
#include "map.cpp"
#include "bsp.cpp"
#include "game.cpp"

i32 main(void)
{
    time_t t;
    srand((unsigned) time(&t));

    HWND window = Win32CreateWindow("BSP_CSG_TEST", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    Win32Renderer renderer = Win32InitD3D11(window, WINDOW_WIDTH, WINDOW_HEIGHT);

    Memory memory = MemoryInit(MEGABYTES(100));    
    Arena gameStateArena = ArenaCreate(&memory, sizeof(GameState));
    Arena resourcesArena = ArenaCreate(&memory, MEGABYTES(25));
    
    GameState *gameState = (GameState *)ArenaPushStruct(&gameStateArena, GameState);

    Win32Shader shader = Win32LoadShader(&renderer, &memory,
                                         "../src/shaders/vertex.hlsl", "../src/shaders/fragment.hlsl");

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
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
         0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    int totalLayoutElements = ARRAY_LENGTH(inputLayoutDesc);
    renderer.device->CreateInputLayout(inputLayoutDesc,
        totalLayoutElements,
        shader.vertexShaderCompiled->GetBufferPointer(),
        shader.vertexShaderCompiled->GetBufferSize(),
        &layout);

    Arena tmpArena = ArenaTmpBegin(&memory, MEGABYTES(10));

    Win32File mapFile = Win32ReadFile("../src/maps/test3.map", &tmpArena);
    
    Map map = {};
    ParseMapFile(&mapFile, &map, &tmpArena);

    MapBrush *brush = &map.entities[0].brushes[0];
    CSGBrush *brushes = GetCSGBrush(brush, &tmpArena, 0, gMapScale);
    for(i32 i = 1; i < map.entities[0].brushesCount; i++)
    {
        brush = &map.entities[0].brushes[i];
        CSGBrushAddBrush(brushes, GetCSGBrush(brush, &tmpArena, 0, gMapScale));
    }

    std::vector<Vertex> vertices;
    CSGBrush *csgBrush = brushes;
    for(i32 b = 0; b < CSGBrushGetNumberOfBrushes(brushes); ++b)
    {
        // Triangulize the poygons
        CSGPoly *csgPoly = csgBrush->polys;
        for(i32 j = 0; j < CSGBrushGetNumberOfPolys(csgBrush); ++j)
        {
            ASSERT(csgPoly->numberOfVertices >= 3);
            for(i32 i = 0; i < csgPoly->numberOfVertices - 2; ++i)
            {
                Vertex a = csgPoly->verts[0];
                Vertex b = csgPoly->verts[i + 1];
                Vertex c = csgPoly->verts[i + 2];
                vertices.push_back(a);
                vertices.push_back(b);
                vertices.push_back(c);
            }
            csgPoly = csgPoly->next;
        }

        csgBrush = csgBrush->next;
    }

    brush = &map.entities[0].brushes[0];
    f32 displacement = -0.1 * gMapScale;
    CSGBrush *disBrushes = GetCSGBrush(brush, &tmpArena, displacement, gMapScale);
    for(i32 i = 1; i < map.entities[0].brushesCount; i++)
    {
        brush = &map.entities[0].brushes[i];
        CSGBrushAddBrush(disBrushes, GetCSGBrush(brush, &tmpArena, displacement, gMapScale));
    }

    CSGPoly *dispPolygons = MergeList(disBrushes, &tmpArena);
    
    std::vector<CSGPoly *> pDispPolygons;
    for(CSGPoly *polygon = dispPolygons; polygon; polygon = polygon->next)
    {
        CSGPolyCalculatePlane(polygon);
        pDispPolygons.push_back(polygon);
    }

    GameInit(&memory);

    // build the bsp tree
    gameState->bspRoot = BuildBSPTree(pDispPolygons, BSP_ROOT, &tmpArena, &resourcesArena);

    ArenaTmpEnd(&memory, &tmpArena);

    i32 verticesCount = vertices.size();
    gameState->vertexBuffer = Win32LoadVertexBuffer(&renderer, vertices.data(), vertices.size(), layout);


    // create the const buffer data to be pass to the gpu
    Vec3 up  = {0, 1,  0};
    gameState->cpuConstBuffer.view = Mat4LookAt(gameState->playerP,
                                                gameState->playerP + gameState->playerDir, up);
    gameState->cpuConstBuffer.proj = Mat4Perspective(60, (f32)WINDOW_WIDTH/(f32)WINDOW_HEIGHT, 0.01f, 100.0f);
    gameState->cpuConstBuffer.wolrd = Mat4Identity();
    gameState->gpuConstBuffer = Win32LoadConstBuffer(&renderer, (void *)&gameState->cpuConstBuffer, sizeof(CBuffer), 0);

    f32 accumulator = 0.0f;
    f32 dt = 1.0f / 120.0f;
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    LARGE_INTEGER lastTime;
    QueryPerformanceCounter(&lastTime);

    ShowWindow(window, 1);
    gRunnig = true;
    while(gRunnig)
    {
        LARGE_INTEGER currTime;
        QueryPerformanceCounter(&currTime);

        f32 deltaTime = (f32)(currTime.QuadPart - lastTime.QuadPart)/(f32)freq.QuadPart;

        Win32FlushEvents(window);

        GameUpdate(&memory, deltaTime);

        accumulator += deltaTime;
        while(accumulator >= dt)
        {
            GameFixUpdate(&memory, dt);
            accumulator -= dt;
        }

        f32 t = accumulator / dt;
        GamePostUpdate(&memory, t);

        // Here goes the rendering code
        float clearColor[] = { 0.2, 0.4, 0.6, 1 };
        renderer.deviceContext->ClearRenderTargetView(renderer.renderTargetView, clearColor);
        renderer.deviceContext->ClearDepthStencilView(
                renderer.depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
        renderer.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        GameRender(&memory, &renderer);

        renderer.swapChain->Present(1, 0);

        gLastInput = gCurrentInput;
        lastTime = currTime;

    }

    GameShutdown(&memory);

    layout->Release();

    Win32UnloadShader(&shader);

    MemoryShutdown(&memory);

    Win32ShutdownD3D11(&renderer);

    return 0;
}

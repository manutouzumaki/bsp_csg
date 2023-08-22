#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <time.h>

#include "defines.h"
#include "math.h"
#include "types.h"

static bool gRunnig;

#include "arena.cpp"
#include "win32.cpp"

struct PolygonData
{
    std::vector<Vertex> vertices; 
};

static void ParseFace(char **current, Face *face, char *end)
{
    // TODO: interchange y and z values
    sscanf(*current, "( %f %f %f ) ( %f %f %f ) ( %f %f %f ) %s [ %f %f %f %f ] [ %f %f %f %f ] %f %f %f",
           &face->a.x, &face->a.z, &face->a.y,
           &face->b.x, &face->b.z, &face->b.y,
           &face->c.x, &face->c.z, &face->c.y,
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

static Vec4 colorList[] = {
    {1, 0, 0, 1},
    {0, 1, 0, 1},
    {0, 0, 1, 1},
    {1, 1, 0, 1},
    {1, 0, 1, 1},
    {0, 1, 1, 1}
};

bool GetIntersection(Vec3 n1, Vec3 n2, Vec3 n3, f32 d1, f32 d2, f32 d3, Vertex *vertex)
{
    f32 denom = Vec3Dot(n1, Vec3Cross(n2, n3));
    if(denom <= VEC_EPSILON && denom >= -VEC_EPSILON)
    {
        return false;
    }
    Vec3 pos = (-d1 * Vec3Cross(n2, n3) -d2 * Vec3Cross(n3, n1) -d3 * Vec3Cross(n1, n2)) / denom;
    i32 colorIndex = rand() % ARRAY_LENGTH(colorList);
    Vec4 col = colorList[colorIndex];
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

Vec3 GetCenterOfPolygon(PolygonData *polygon)
{
    Vec3 center = {};
    for(i32 i = 0; i < polygon->vertices.size(); ++i)
    {
        center = center + polygon->vertices[i].position;
    }
    center = center / polygon->vertices.size();
    return center;
}

std::vector<Vertex> TriangulateBrush(Brush *brush)
{
    Face *faces = brush->faces;
    u32 facesCount = brush->facesCount;

    // TODO: change the vector to darrays using memory arenas
    PolygonData polygons[32];
    i32 polygonsCount = facesCount;
    for(i32 i = 0; i < facesCount-2; ++i) {
        for(i32 j = i;j < facesCount-1; ++j) {
            for(i32 k = j; k < facesCount; ++k) {

                if(i != j && i != k && j != k)
                {
                    Plane a = GetPlaneFromThreePoints(faces[i].a, faces[i].b, faces[i].c); 
                    Plane b = GetPlaneFromThreePoints(faces[j].a, faces[j].b, faces[j].c); 
                    Plane c = GetPlaneFromThreePoints(faces[k].a, faces[k].b, faces[k].c); 
                    Vertex vertex = {};
                    if(GetIntersection(a.n, b.n, c.n, a.d, b.d, c.d, &vertex))
                    {
                        bool illegal = false;
                        for(i32 m = 0; m < facesCount; ++m)
                        {
                            Plane plane = GetPlaneFromThreePoints(faces[m].a, faces[m].b, faces[m].c); 
                            f32 dot = Vec3Dot(plane.n, vertex.position);
                            f32 d = plane.d;
                            if((dot + d) > VEC_EPSILON)
                            {
                                illegal = true;
                            }
                        }
                        if(illegal == false)
                        {
                            // TODO: see where this should be performed
                            Mat4 scale = Mat4Scale(1.0f/128.0f, 1.0f/128.0f, 1.0f/128.0f);
                            vertex.position = Mat4TransformPoint(scale, vertex.position);
                            polygons[i].vertices.push_back(vertex);
                            polygons[j].vertices.push_back(vertex);
                            polygons[k].vertices.push_back(vertex);
                        }
                    }
                }
            }
        }
    }
    
    // order the vertices in the polygons
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        Plane polygonPlane = GetPlaneFromThreePoints(faces[p].a, faces[p].b, faces[p].c); 
        PolygonData *polygon = polygons + p;
        Vec3 center = GetCenterOfPolygon(polygon);
        
        for(i32 n = 0; n <= polygon->vertices.size() - 3; ++n)
        {
            Vec3 a = Vec3Normalized(polygon->vertices[n].position - center);
            Plane p = GetPlaneFromThreePoints(polygon->vertices[n].position,
                                              center, center + polygonPlane.n);

            f32 smallestAngle = -1;
            i32 smallest = -1;

            for(i32 m = n + 1; m <= polygon->vertices.size() - 1; ++m)
            {
                Vertex vertex = polygon->vertices[m];
                if((Vec3Dot(p.n, vertex.position) + p.d) > 0.0f)
                {
                    Vec3 b = Vec3Normalized(vertex.position - center);
                    f32 angle = Vec3Dot(a, b);
                    if(angle > smallestAngle)
                    {
                        smallestAngle = angle;
                        smallest = m;
                    }
                }
            }

            if(smallest >= 0)
            {
                Vertex tmp = polygon->vertices[n + 1];
                polygon->vertices[n + 1] = polygon->vertices[smallest];
                polygon->vertices[smallest] = tmp;
            }
        } 
    }

    // TODO: Reverse vertices that are in inver order for back face culling
#if 0
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        Plane facePlane = GetPlaneFromThreePoints(faces[p].a, faces[p].b, faces[p].c); 
        PolygonData *polygon = polygons + p;
        Vec3 a = polygon->vertices[0].position;
        Vec3 b = polygon->vertices[1].position;
        Vec3 c = polygon->vertices[2].position;
        
        Vec3 ab = Vec3Normalized(b - a);
        Vec3 ac = Vec3Normalized(c - a);
        Vec3 n = Vec3Cross(ab, ac);
        if(Vec3Dot(n, facePlane.n) < VEC_EPSILON)
        {
            std::vector<Vertex> tmpVertices = polygon->vertices;
            for(i32 i = 0; i > tmpVertices.size(); ++i)
            {
                polygon->vertices[i] = tmpVertices[(tmpVertices.size()-1)-i];
            }
        }
    }
#endif

    // TODO: try to draw the brush it should be a green cube
    
    // Triangulize the poygons
    std::vector<Vertex> vertices;
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        PolygonData *polygon = polygons + p;
        for(i32 i = 0; i < polygon->vertices.size() - 2; ++i)
        {
            Vertex a = polygon->vertices[0];
            Vertex b = polygon->vertices[i + 1];
            Vertex c = polygon->vertices[i + 2];
            vertices.push_back(a);
            vertices.push_back(b);
            vertices.push_back(c);
        }
    }
    return vertices;
}

i32 main(void)
{
   time_t t;
   srand((unsigned) time(&t));


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

    // create the const buffer data to be pass to the gpu
    Vec3 pos = {0, 1.2, -1.5};
    Vec3 tar = {0, 1,  0};
    Vec3 up  = {0, 1,  0};
    CBuffer cbuffer = {};
    cbuffer.view = Mat4LookAt(pos, tar, up);
    cbuffer.proj = Mat4Perspective(60, (f32)WINDOW_WIDTH/(f32)WINDOW_HEIGHT, 0.01f, 100.0f);
    cbuffer.wolrd = Mat4Identity();
    Win32ConstBuffer constBuffer = Win32LoadConstBuffer(&renderer, (void *)&cbuffer, sizeof(cbuffer), 0);

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

    // TODO: try to parse the valve's .map file
    Win32File mapFile = Win32ReadFile("../src/test.map", &resourcesArena);
    ParseMapFile(&mapFile, gameState);

    Map *map = &gameState->map;
    std::vector<Vertex> vertices;
    for(i32 i = 0; i < map->entities[0].brushesCount; i++)
    {
        Brush *brush = &map->entities[0].brushes[i];
        std::vector<Vertex> brushVertices = TriangulateBrush(brush);
        vertices.insert(vertices.end(), brushVertices.begin(), brushVertices.end());
    }
    i32 verticesCount = vertices.size();
    Win32VertexBuffer vertexBuffer = Win32LoadVertexBuffer(&renderer, vertices.data(), vertices.size(), layout);
    


    gRunnig = true;
    while(gRunnig)
    {
        Win32FlushEvents(window);

        static f32 angle = 0;
        Mat4 rotY = Mat4RotateY(angle);
        cbuffer.wolrd = rotY;
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

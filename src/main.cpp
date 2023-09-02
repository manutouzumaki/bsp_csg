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
#include "bsp.h"

static bool gRunnig;
static InputState gCurrentInput;
static InputState gLastInput;

static f32 mapScale = 128.0f;

#include "arena.cpp"
#include "win32.cpp"
#include "input.cpp"
#include "csg.cpp"
#include "bsp.cpp"

static Vertex cubeVertices[] = {
    -0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,

     0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
     0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, 0, 1, 0, 1, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, 0, 1, 0, 1, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 0, 1, 0, 1, 0.0f, 1.0f
};

static void ParseFace(char **current, MapFace *face, char *end)
{
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

static void ParseBrush(char **current, MapBrush *brush, char *end, Arena *arena)
{

    brush->faces = (MapFace *)ArenaPushArray(arena, 32, MapFace);
    
    while(*current != end)
    {
        if(**current == '(')
        {
            MapFace *face = brush->faces + brush->facesCount++;
            ParseFace(current, face, end);
        }

        (*current)++;
        if(**current == '}')
        {
            return;
        }
    }
}

static void ParseEntity(char **current, MapEntity *entity, char *end, Arena *arena)
{

    entity->brushes = (MapBrush *)ArenaPushArray(arena, 32, MapBrush);

    while(*current != end)
    {
        if(**current == '{')
        {
           MapBrush *brush = entity->brushes + entity->brushesCount++; 
            (*current)++;
            ParseBrush(current, brush, end, arena);
        }

        (*current)++;
        if(**current == '}')
        {
            return;
        }
    }
}

static void ParseMapFile(Win32File *file, Map *map, Arena *arena)
{
    char *start = (char *)file->data;
    char *end = (char *)file->data + file->size;
    char *current = start;

    map->entities = (MapEntity *)ArenaPushArray(arena, 1, MapEntity);

    while(current != end)
    {
        if(*current == '{')
        {
            MapEntity *entity = map->entities + map->entitiesCount++;
            current++;
            ParseEntity(&current, entity, end, arena);
        }

        current++;
        if(*current == '}')
        {
            return;
        }
    }
}

static bool GetIntersection(Vec3 n1, Vec3 n2, Vec3 n3, f32 d1, f32 d2, f32 d3, Vertex *vertex)
{
    f32 denom = Vec3Dot(n1, Vec3Cross(n2, n3));
    if(denom <= FLT_EPSILON && denom >= -FLT_EPSILON)
    {
        return false;
    }
    Vec3 pos = (-d1 * Vec3Cross(n2, n3) -d2 * Vec3Cross(n3, n1) -d3 * Vec3Cross(n1, n2)) / denom;
    f32 r = InvLerp(0, 20, rand()%20);
    f32 g = InvLerp(0, 20, rand()%20);
    f32 b = InvLerp(0, 20, rand()%20);
    Vec4 col = {r, g, b, 1.0f};
    *vertex = {pos, col};
    return true;
}

static Plane GetPlaneFromThreePoints(Vec3 a, Vec3 b, Vec3 c)
{
    Plane plane;
    Vec3 ab = b - a;
    Vec3 ac = c - a;
    Vec3 n = Vec3Normalized(Vec3Cross(ab, ac));
    f32 d = -n.x*a.x -n.y*a.y -n.z*a.z;
    return {n, d};
}

static Vec3 GetCenterOfPolygon(PolygonData *polygon)
{
    Vec3 center = {};
    for(i32 i = 0; i < polygon->vertices.size(); ++i)
    {
        center = center + polygon->vertices[i].position;
    }
    center = center / polygon->vertices.size();
    return center;
}

static CSGBrush *GetCSGBrush(MapBrush *brush, Arena *arena, f32 displacement)
{
    MapFace *faces = brush->faces;
    u32 facesCount = brush->facesCount;

    // TODO: change the vector to darrays using memory arenas
    std::vector<PolygonData> polygons;
    polygons.resize(facesCount);
    i32 polygonsCount = facesCount;
    for(i32 i = 0; i < facesCount-2; ++i) {
        for(i32 j = i;j < facesCount-1; ++j) {
            for(i32 k = j; k < facesCount; ++k) {

                if(i != j && i != k && j != k)
                {
                    Plane a = GetPlaneFromThreePoints(faces[i].a, faces[i].b, faces[i].c); 
                    Plane b = GetPlaneFromThreePoints(faces[j].a, faces[j].b, faces[j].c); 
                    Plane c = GetPlaneFromThreePoints(faces[k].a, faces[k].b, faces[k].c); 
                    a.d += displacement;
                    b.d += displacement;
                    c.d += displacement;
                    Vertex vertex = {};
                    if(GetIntersection(a.n, b.n, c.n, a.d, b.d, c.d, &vertex))
                    {
                        bool illegal = false;
                        for(i32 m = 0; m < facesCount; ++m)
                        {
                            Plane plane = GetPlaneFromThreePoints(faces[m].a, faces[m].b, faces[m].c); 
                            plane.d += displacement;
                            f32 dot = Vec3Dot(plane.n, vertex.position);
                            f32 d = plane.d;
                            if((dot + d) > EPSILON)
                            {
                                illegal = true;
                            }
                        }
                        if(illegal == false)
                        {
                            // TODO: see where this should be performed
                            Mat4 scale = Mat4Scale(1.0f/mapScale, 1.0f/mapScale, 1.0f/mapScale);
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

    // TODO: get texture coords
    
    // order the vertices in the polygons
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        Plane polygonPlane = GetPlaneFromThreePoints(faces[p].a, faces[p].b, faces[p].c); 
        PolygonData *polygon = polygons.data() + p;

        ASSERT(polygon->vertices.size() >= 3);

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

    CSGBrush *csgBrush = (CSGBrush *)ArenaPushStruct(arena, CSGBrush);
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        PolygonData *polygon = polygons.data() + p;
        CSGPoly *csgPoly = (CSGPoly *)ArenaPushStruct(arena, CSGPoly);
        for(i32 v = 0; v < polygon->vertices.size(); ++v)
        {
            CSGPolyAddVertex(csgPoly, polygon->vertices[v], arena);
        }
        CSGPolyCalculatePlane(csgPoly);
        CSGBrushAddPoly(csgBrush, csgPoly);
        CSGBrushCalculateAABB(csgBrush);
    }

    return csgBrush;

}

static int PointInSolidSpace(BSPNode *node, Vec3 p)
{
    if(node == 0) return POINT_IN_FRONT_OF_PLANE;

    while(!BSPNodeIsLeaf(node))
    {
        f32 dist = Vec3Dot(node->plane.n, p) + node->plane.d;
        node = node->child[dist < FLT_EPSILON];
    }
    // Now at a leaf, inside/outside status determined by solid flag
    return BSPNodeIsSolid(node) ? POINT_BEHIND_PLANE : POINT_IN_FRONT_OF_PLANE;
}

static int IntersectionLinePlane(Vec3 a, Vec3 b, Plane p, f32 &t, Vec3 &q)
{
    Vec3 ab = b - a;
    t = (-p.d - Vec3Dot(p.n, a)) / Vec3Dot(p.n, ab);
    if(t >= 0.0f && t <= 1.0f)
    {
        q = a + ab * t;
        return 1;
    }
    return 0;
}

static int RayIntersect(BSPNode *node, Vec3 a, Vec3 b, Vec3 *outpoint, Plane *outplane)
{
    if(node == 0) return 0;
    if(BSPNodeIsLeaf(node))
    {
        if(BSPNodeIsSolid(node))
        {
            if(Vec3LenSq(outplane->n) > FLT_EPSILON)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    if(node->plane.n.y == 1.0f && node->plane.d < 0 && node->plane.d > -1.06 && Vec3LenSq(b - a) > 0)
    {
        i32 StopHere = 0;
    }
    // TODO: play with this EPSILON
    i32 aPoint = (i32)(((Vec3Dot(node->plane.n, a) + node->plane.d)) < 0);
    i32 bPoint = (i32)(((Vec3Dot(node->plane.n, b) + node->plane.d)) < 0);
    
    if(aPoint == POINT_IN_FRONT_OF_PLANE && bPoint == POINT_IN_FRONT_OF_PLANE)
    {
        // the ray is infront of the plane we travers the front side
        return RayIntersect(node->front, a, b, outpoint, outplane);
    }
    else if(aPoint == POINT_BEHIND_PLANE && bPoint == POINT_BEHIND_PLANE)
    {
        // the ray is behind of the plane we travers the back side 
        return RayIntersect(node->back, a, b, outpoint, outplane);
    }
    else
    {
        // here the ray intersect the plane, so we check for the intersection and split the ray
        f32 t = -1.0f;
        Vec3 q;
        if(IntersectionLinePlane(a, b, node->plane, t, q))
        {
            bool frontResult, backResult;
            if(aPoint == POINT_IN_FRONT_OF_PLANE)
            {
                *outpoint = q;
                *outplane = node->plane;

                // check the front part
                frontResult = RayIntersect(node->front, a, q, outpoint, outplane);
                // check the back part
                backResult = RayIntersect(node->back, q, b, outpoint, outplane);
            }
            else
            {
                // check the back part
                frontResult = RayIntersect(node->back, a, q, outpoint, outplane);
                // check the front part
                backResult = RayIntersect(node->front, q, b, outpoint, outplane);
            }
            return frontResult || backResult;
        }
        else
        {
            return 0;
        }

    }
}

i32 main(void)
{
    time_t t;
    srand((unsigned) time(&t));

    HWND window = Win32CreateWindow("BSP_CSG_TEST", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    Win32Renderer renderer = Win32InitD3D11(window, WINDOW_WIDTH, WINDOW_HEIGHT);

    Memory memory = MemoryInit(MEGABYTES(100));    
    Arena gameStateArena = ArenaCreate(&memory, sizeof(GameState));
    Arena resourcesArena = ArenaCreate(&memory, MEGABYTES(25));
    
    GameState *gameState = (GameState *)ArenaPushStruct(&resourcesArena, GameState);

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

    Win32File mapFile = Win32ReadFile("../src/maps/test.map", &resourcesArena);

    Arena tmpArena = ArenaTmpBegin(&memory, MEGABYTES(10));
    
    Map map = {};
    ParseMapFile(&mapFile, &map, &tmpArena);

    MapBrush *brush = &map.entities[0].brushes[0];
    CSGBrush *brushes = GetCSGBrush(brush, &tmpArena, 0);
    for(i32 i = 1; i < map.entities[0].brushesCount; i++)
    {
        brush = &map.entities[0].brushes[i];
        CSGBrushAddBrush(brushes, GetCSGBrush(brush, &tmpArena, 0));
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
    f32 displacement = -0.1 * mapScale;
    CSGBrush *disBrushes = GetCSGBrush(brush, &tmpArena, displacement);
    for(i32 i = 1; i < map.entities[0].brushesCount; i++)
    {
        brush = &map.entities[0].brushes[i];
        CSGBrushAddBrush(disBrushes, GetCSGBrush(brush, &tmpArena, displacement));
    }

    CSGPoly *dispPolygons = MergeList(disBrushes, &tmpArena);
    
    std::vector<CSGPoly *> pDispPolygons;
    for(CSGPoly *polygon = dispPolygons; polygon; polygon = polygon->next)
    {
        CSGPolyCalculatePlane(polygon);
        pDispPolygons.push_back(polygon);
    }

    // build the bsp tree
    BSPNode *bspRoot = BuildBSPTree(pDispPolygons, BSP_ROOT, &tmpArena, &resourcesArena);

    ArenaTmpEnd(&memory, &tmpArena);

    i32 verticesCount = vertices.size();
    Win32VertexBuffer vertexBuffer = Win32LoadVertexBuffer(&renderer, vertices.data(), vertices.size(), layout);

    Win32VertexBuffer cubeBuffer = Win32LoadVertexBuffer(&renderer, cubeVertices, ARRAY_LENGTH(cubeVertices), layout);

    f32 rotationY = 0.0f;
    Vec3 playerP = {1.2, 0.6, -1.8};
    Vec3 playerV = {0, 0, 0};
    Vec3 playerA = {0, -1, 0};

    // create the const buffer data to be pass to the gpu
    Vec3 dir = {0, 0,  1};
    Vec3 up  = {0, 1,  0};
    CBuffer cbuffer = {};
    cbuffer.view = Mat4LookAt(playerP, playerP + dir, up);
    cbuffer.proj = Mat4Perspective(80, (f32)WINDOW_WIDTH/(f32)WINDOW_HEIGHT, 0.01f, 100.0f);
    cbuffer.wolrd = Mat4Identity();
    Win32ConstBuffer constBuffer = Win32LoadConstBuffer(&renderer, (void *)&cbuffer, sizeof(cbuffer), 0);


    ShowWindow(window, 1);
    gRunnig = true;
    while(gRunnig)
    {
        Win32FlushEvents(window);

        rotationY = 0;
        if(KeyPress(VK_LEFT))
        {
            rotationY = 0.016 * 2;
        }
        if(KeyPress(VK_RIGHT))
        {
            rotationY = -0.016 * 2;
        }
        dir = Mat4TransformVector(Mat4RotateY(rotationY), dir);
        Vec3 right = Vec3Normalized(Vec3Cross(dir, up));

        Vec3 velocity = {};
        if(KeyPress('A'))
        {
            velocity = velocity + right;
        }
        if(KeyPress('D'))
        {
            velocity = velocity - right; 
        }
        if(KeyPress('W'))
        {
            velocity = velocity + dir;
        }
        if(KeyPress('S'))
        {
            velocity = velocity - dir;
        }
        if(KeyPress('R'))
        {
            velocity.y += 1.0;
        }
        if(KeyPress('F'))
        {
            velocity.y -= 1.0;
        }
        

        velocity = Vec3Normalized(velocity) * 0.016 * 2;

        Vec3 q;
        Plane plane;
        i32 iterations = 100;

        i32 collisionCount = RayIntersect(bspRoot, playerP, playerP + velocity, &q, &plane);
        while(Vec3LenSq(velocity) > FLT_EPSILON && iterations >= 0)
        {
            if(collisionCount)
            {
                Vec3 n = plane.n;
                Vec3 newP =  q + (n * 0.002f);
                if(!PointInSolidSpace(bspRoot, newP))
                {
                    playerP = newP;
                    velocity = velocity - (n * Vec3Dot(velocity, n));
                    collisionCount = RayIntersect(bspRoot, playerP, playerP + velocity, &q, &plane);
                }
            }
            else
            {
                Vec3 newP = playerP + velocity;
                if(!PointInSolidSpace(bspRoot, newP))
                {
                    playerP = newP;
                }
                break;
            }
            iterations--;
        }

        //Vec3 cameraP = (playerP - dir * 1.0f) + up * 0.3;
        //cbuffer.view = Mat4LookAt(cameraP, playerP, up);
        Vec3 cameraP = playerP + up * 0.2;
        cbuffer.view = Mat4LookAt(cameraP, cameraP + dir, up);
        Win32UpdateConstBuffer(&renderer, &constBuffer, (void *)&cbuffer);

        float clearColor[] = { 0.2, 0.2, 0.2, 1 };
        renderer.deviceContext->ClearRenderTargetView(
                renderer.renderTargetView, clearColor);

        renderer.deviceContext->ClearDepthStencilView(
                renderer.depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Here goes the rendering code
        u32 stride = sizeof(Vertex);
        u32 offset = 0;
        renderer.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cbuffer.wolrd = Mat4Identity();
        Win32UpdateConstBuffer(&renderer, &constBuffer, (void *)&cbuffer);

        renderer.deviceContext->IASetInputLayout(vertexBuffer.layout);
        renderer.deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer.GPUBuffer, &stride, &offset);
        renderer.deviceContext->Draw(vertexBuffer.verticesCount, 0);

        //cbuffer.wolrd = Mat4Translate(playerP.x, playerP.y, playerP.z) * Mat4Scale(0.2, 0.2, 0.2);
        //Win32UpdateConstBuffer(&renderer, &constBuffer, (void *)&cbuffer);

        //renderer.deviceContext->IASetInputLayout(cubeBuffer.layout);
        //renderer.deviceContext->IASetVertexBuffers(0, 1, &cubeBuffer.GPUBuffer, &stride, &offset);
        //renderer.deviceContext->Draw(cubeBuffer.verticesCount, 0);

        renderer.swapChain->Present(1, 0);

        gLastInput = gCurrentInput;

    }

    Win32UnloadConstBuffer(&constBuffer);

    Win32UnloadVertexBuffer(&vertexBuffer);
    layout->Release();

    Win32UnloadShader(&shader);

    MemoryShutdown(&memory);

    Win32ShutdownD3D11(&renderer);

    return 0;
}

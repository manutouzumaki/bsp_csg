#include <windows.h>
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
#include "poly.h"
#include "brush.h"

static bool gRunnig;

#include "poly.cpp"
#include "brush.cpp"
#include "arena.cpp"
#include "win32.cpp"

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

bool GetIntersection(Vec3 n1, Vec3 n2, Vec3 n3, f32 d1, f32 d2, f32 d3, Vertex *vertex)
{
    f32 denom = Vec3Dot(n1, Vec3Cross(n2, n3));
    if(denom <= VEC_EPSILON && denom >= -VEC_EPSILON)
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

IBrush *GetIBrush(Brush *brush)
{
    Face *faces = brush->faces;
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

    // TODO: get texture coords
    
    // order the vertices in the polygons
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        Plane polygonPlane = GetPlaneFromThreePoints(faces[p].a, faces[p].b, faces[p].c); 
        PolygonData *polygon = polygons.data() + p;
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

    IBrush *iBrush = new IBrush;
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        PolygonData *polygon = polygons.data() + p;
        Poly *poly = new Poly;
        for(i32 v = 0; v < polygon->vertices.size(); ++v)
        {
            poly->AddVertex(polygon->vertices[v]);
        }
        poly->CalculatePlane();
        iBrush->AddPoly(poly);
        iBrush->CalculateAABB();
    }

    return iBrush;

}

BSPNode::BSPNode(BSPType type)
{
    this->plane = {};
    this->back = 0;
    this->front = 0;
    this->type = type;
}

BSPNode::BSPNode(BSPNode *front, BSPNode *back, Plane plane)
{
    this->plane = plane;
    this->front = front;
    this->back = back;
    this->type = BSP_TYPE_NODE;
}

int ClassifyPointToPlane(Vec3 p, Plane plane)
{
    // Compute signed distance of point from plane
    float dist = Vec3Dot(plane.n, p) + plane.d;
    // Classify p based on the signed distance
    if (dist > VEC_EPSILON)
        return POINT_IN_FRONT_OF_PLANE;
    if (dist < -VEC_EPSILON)
        return POINT_BEHIND_PLANE;
    return POINT_ON_PLANE;
}

int ClassifyPolygonToPlane(Poly *poly, Plane plane)
{
    // Loop over all polygon vertices and count how many vertices
    // lie in front of and how many lie behind of the thickened plane
    int numInFront = 0, numBehind = 0;
    int numVerts = poly->numberOfVertices;

    for (int i = 0; i < numVerts; i++) 
    {
        Vec3 p = poly->verts[i].position;
        switch (ClassifyPointToPlane(p, plane)) 
        {
            case POINT_IN_FRONT_OF_PLANE:
                numInFront++;
                break;
            case POINT_BEHIND_PLANE:
                numBehind++;
                break;
        }
    }
    // If vertices on both sides of the plane, the polygon is straddling
    if (numBehind != 0 && numInFront != 0)
    return POLYGON_STRADDLING_PLANE;
    // If one or more vertices in front of the plane and no vertices behind
    // the plane, the polygon lies in front of the plane
    if (numInFront != 0)
    return POLYGON_IN_FRONT_OF_PLANE;
    // Ditto, the polygon lies behind the plane if no vertices in front of
    // the plane, and one or more vertices behind the plane
    if (numBehind != 0)
    return POLYGON_BEHIND_PLANE;
    // All vertices lie on the plane so the polygon is coplanar with the plane
    return POLYGON_COPLANAR_WITH_PLANE;
}

Plane PickSplittingPlane(std::vector<Poly *> &polygons)
{
    // Blend factor for optimizing for balance or split (should be tweaked)
    const f32 K = 0.8f;

    Plane bestPlane;
    f32 bestScore = FLT_MAX;

    // try the plane of each polygon as a dividing plane
    for(i32 i = 0; i < polygons.size(); i++)
    {
        i32 numInFront = 0, numBehind = 0, numStraddling = 0;
        Plane plane = polygons[i]->plane;
        // test against all other polygons
        for(i32 j = 0; j < polygons.size(); j++)
        {
            if(i == j) continue;
            // Keep standing count of the various poly-plane relationships
            switch (ClassifyPolygonToPlane(polygons[j], plane))
            {
                case POLYGON_COPLANAR_WITH_PLANE:
                /* Coplanar polygons treated as being in front of plane */
                case POLYGON_IN_FRONT_OF_PLANE:
                    numInFront++;
                    break;
                case POLYGON_BEHIND_PLANE:
                    numBehind++;
                    break;
                case POLYGON_STRADDLING_PLANE:
                    numStraddling++;
                    break;
            }
        }
        
        // Compute score as a weighted combination (based on K, with K in range
        // 0..1) between balance and splits (lower score is better)
        f32 score = K * numStraddling + (1.0f - K) * fabs(numInFront - numBehind);
        if (score < bestScore) 
        {
            bestScore = score;
            bestPlane = plane;
        }
    }
    return bestPlane;
}

void SplitPolygon(Poly *poly, Plane plane, Poly **frontPoly, Poly **backPoly)
{
    int numFront = 0, numBack = 0;
    Vec3 frontVerts[100], backVerts[100];
    // Test all edges (a, b) starting with edge from last to first vertex
    int numVerts = poly->numberOfVertices;
    Vec3 a = poly->verts[numVerts - 1].position;
    int aSide = ClassifyPointToPlane(a, plane);
    // Loop over all edges given by vertex pair (n - 1, n)
    for (int n = 0; n < numVerts; n++) 
    {
        Vec3 b = poly->verts[n].position;
        int bSide = ClassifyPointToPlane(b, plane);
        if (bSide == POINT_IN_FRONT_OF_PLANE)
        {
            if (aSide == POINT_BEHIND_PLANE)
            {
                // Edge (a, b) straddles, output intersection point to both sides
                Vec3 i; // IntersectEdgeAgainstPlane(a, b, plane);
                f32 t = -1.0f;
                PlaneGetIntersection(plane, a, b, i, t);
                ASSERT(ClassifyPointToPlane(i, plane) == POINT_ON_PLANE);
                frontVerts[numFront++] = backVerts[numBack++] = i;
            }
            // In all three cases, output b to the front side
            frontVerts[numFront++] = b;
        }
        else if (bSide == POINT_BEHIND_PLANE) 
        {
            if (aSide == POINT_IN_FRONT_OF_PLANE)
            {
            // Edge (a, b) straddles plane, output intersection point
            Vec3 i; // IntersectEdgeAgainstPlane(a, b, plane);
            f32 t = -1.0f;
            PlaneGetIntersection(plane, a, b, i, t);
            ASSERT(ClassifyPointToPlane(i, plane) == POINT_ON_PLANE);
            frontVerts[numFront++] = backVerts[numBack++] = i;
            }
            else if (aSide == POINT_ON_PLANE)
            {
            // Output a when edge (a, b) goes from ‘on’ to ‘behind’ plane
            backVerts[numBack++] = a;
            }
            // In all three cases, output b to the back side
            backVerts[numBack++] = b;
        }
        else
        {
            // b is on the plane. In all three cases output b to the front side
            frontVerts[numFront++] = b;
            // In one case, also output b to back side
            if (aSide == POINT_BEHIND_PLANE)
                backVerts[numBack++] = b;
        }
        // Keep b as the starting point of the next edge
        a = b;
        aSide = bSide;
    }
    // Create (and return) two new polygons from the two vertex lists
    *frontPoly = new Poly;
    *backPoly = new Poly;
    for(i32 i = 0; i < numFront; i++)
    {
        Vertex vertex;
        vertex.position = frontVerts[i];
        vertex.color = {1, 0, 0, 1};
        (*frontPoly)->AddVertex(vertex);
    }
    for(i32 i = 0; i < numBack; i++)
    {
        Vertex vertex;
        vertex.position = backVerts[i];
        vertex.color = {1, 0, 0, 1};
        (*backPoly)->AddVertex(vertex);
    }
    (*frontPoly)->CalculatePlane();
    (*backPoly)->CalculatePlane();
}

BSPNode *BuildBSPTree(std::vector<Poly *> &polygons, BSPState state)
{
    // Return null is there are no polys
    //if(polygons.empty()) return 0;

    i32 numPolygons = polygons.size();
    
    // If criterion for a leaf is matche, create a leaf node from remaining polygons
    if(numPolygons <= 0)
    {
        // TODO: determin if this leaf is solid or not
        if(state == BSP_FRONT)
        {
            return new BSPNode(BSP_TYPE_EMPTY);
        }
        else if(state == BSP_BACK)
        {
            return new BSPNode(BSP_TYPE_SOLID);
        }
    }
    // Select best possible partitioning plane base on the input geometry
    Plane splitPlane = PickSplittingPlane(polygons);

    std::vector<Poly *> frontList, backList;

    // Test each polygon against the divding plane, adding them to the front list, back list, or both
    // as appropiate
    for(i32 i = 0; i < numPolygons; i++)
    {
        Poly *poly = polygons[i], *frontPart, *backPart;
        switch(ClassifyPolygonToPlane(poly, splitPlane))
        {
            case POLYGON_COPLANAR_WITH_PLANE:
                // What’s done in this case depends on what type of tree is being
                // built. For a node-storing tree, the polygon is stored inside
                // the node at this level (along with all other polygons coplanar
                // with the plane). Here, for a leaf-storing tree, coplanar polygons
                // are sent to either side of the plane. In this case, to the front
                // side, by falling through to the next case
                break;
            case POLYGON_IN_FRONT_OF_PLANE:
                frontList.push_back(poly);
                break;
            case POLYGON_BEHIND_PLANE:
                backList.push_back(poly);
                break;
            case POLYGON_STRADDLING_PLANE:
                // Split polygon to plane and send a part to each side of the plane
                SplitPolygon(poly, splitPlane, &frontPart, &backPart);
                frontList.push_back(frontPart);
                frontList.push_back(backPart);
                break;
        }
    }

    // Recursively build child subtrees and return new tree root combining them
    BSPNode *frontTree = BuildBSPTree(frontList, BSP_FRONT);
    BSPNode *backTree  = BuildBSPTree(backList, BSP_BACK);
    return new BSPNode(frontTree, backTree, splitPlane);
}

int RayIntersect(BSPNode *node, Vec3 p, Vec3 d, f32 tmin, f32 tmax, f32 *thit)
{
    std::stack<BSPNode *> nodeStack;
    std::stack<float> timeStack;
    ASSERT(node != NULL);
    while (1) 
    {
        if (!node->IsLeaf())
        {
            float denom = Vec3Dot(node->plane.n, d);
            float dist = node->plane.d - Vec3Dot(node->plane.n, p);
            int nearIndex = dist > 0.0f;
            // If denom is zero, ray runs parallel to plane. In this case,
            // just fall through to visit the near side (the one p lies on)
            if (denom != 0.0f) 
            {
                float t = dist / denom;
                if (0.0f <= t && t <= tmax) 
                {
                    if (t >= tmin)
                    {
                        // Straddling, push far side onto stack, then visit near side
                        nodeStack.push(node->child[1 ^ nearIndex]);
                        timeStack.push(tmax);
                        tmax = t;
                    } 
                    else 
                        nearIndex = 1 ^ nearIndex;
                }
            }
            node = node->child[nearIndex];
        }
        else
        {
            // Now at a leaf. If it is solid, there’s a hit at time tmin, so exit
            if (node->IsSolid())
            {
                *thit = tmin;
                return 1;
            }
            // Exit if no more subtrees to visit, else pop off a node and continue
            if (nodeStack.empty()) break;

            tmin = tmax;
            node = nodeStack.top(); nodeStack.pop();
            tmax = timeStack.top(); timeStack.pop();
        }
    }
    // No hit
    return 0;
}

i32 main(void)
{
   time_t t;
   srand((unsigned) time(&t));


    HWND window = Win32CreateWindow("BSP_CSG_TEST", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    Win32Renderer renderer = Win32InitD3D11(window, WINDOW_WIDTH, WINDOW_HEIGHT);

    Memory memory = MemoryInit(MEGABYTES(100));

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
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
         0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
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

    Brush *brush = &map->entities[0].brushes[0];
    IBrush *brushes = GetIBrush(brush);
    for(i32 i = 1; i < map->entities[0].brushesCount; i++)
    {
        brush = &map->entities[0].brushes[i];
        brushes->AddBrush(GetIBrush(brush));
    }

    Poly *polygons = brushes->MergeList();

    // Triangulize the poygons
    std::vector<Vertex> vertices;
    for(Poly *polygon = polygons; polygon; polygon = polygon->next)
    {
        for(i32 i = 0; i < polygon->numberOfVertices - 2; ++i)
        {
            Vertex a = polygon->verts[0];
            Vertex b = polygon->verts[i + 1];
            Vertex c = polygon->verts[i + 2];
            vertices.push_back(a);
            vertices.push_back(b);
            vertices.push_back(c);
        }
    }

    // TODO: try to build the bsp tree
    std::vector<Poly *> pPolygons;
    for(Poly *polygon = polygons; polygon; polygon = polygon->next)
    {
        pPolygons.push_back(polygon);
    }

    BSPNode *bspRoot = BuildBSPTree(pPolygons, BSP_ROOT);

    i32 verticesCount = vertices.size();
    Win32VertexBuffer vertexBuffer = Win32LoadVertexBuffer(&renderer, vertices.data(), vertices.size(), layout);


    ShowWindow(window, 1);
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

#if 0
CSGBrush GetCSGBrush(Brush *brush, Arena *arena)
{
    Face *faces = brush->faces;
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

    // TODO: get texture coords
    
    // order the vertices in the polygons
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        Plane polygonPlane = GetPlaneFromThreePoints(faces[p].a, faces[p].b, faces[p].c); 
        PolygonData *polygon = polygons.data() + p;
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

    // calculate the polygons planes
    for(i32 p = 0; p < polygonsCount; ++p)
    {
        PolygonData *polygon = polygons.data() + p;
        polygon->plane = GetPlaneFromThreePoints(faces[p].a, faces[p].b, faces[p].c); 
    }

    CSGBrush csgBrush;
    csgBrush.polygons = polygons;
    return csgBrush;

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
}
#endif

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

static CSGBrush *GetCSGBrush(MapBrush *brush, Arena *arena, f32 displacement, f32 scale)
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
                            Mat4 scaleMat = Mat4Scale(1.0f/scale, 1.0f/scale, 1.0f/scale);
                            vertex.position = Mat4TransformPoint(scaleMat, vertex.position);
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

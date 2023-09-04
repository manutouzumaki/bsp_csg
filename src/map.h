#ifndef _MAP_H_
#define _MAP_H_

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

struct PolygonData
{
    std::vector<Vertex> vertices; 
    Plane plane;
};

#endif

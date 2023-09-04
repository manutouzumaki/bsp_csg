#ifndef _CSG_H_
#define _CSG_H_

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

struct CSGPoly
{
    Vertex *verts;
    Plane plane;
    CSGPoly *next;
    i32 numberOfVertices;
};

struct CSGBrush
{
    Vec3 min, max;
    CSGBrush *next;
    CSGPoly *polys;
};

#endif

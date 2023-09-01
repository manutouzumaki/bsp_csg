#ifndef _CSG_H_
#define _CSG_H_

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

#ifndef _POLY_H_
#define _POLY_H_

struct  Poly
{
    Vertex *verts;
    Plane plane;
    Poly *next;
    i32 numberOfVertices;

    Poly *CopyList();
    Poly *CopyPoly();
    Poly *ClipToList(Poly *poly, bool bClipOnPlane);
    void AddVertex(Vertex &vertex);
    void AddPoly(Poly *poly);

    bool CalculatePlane();
    void SplitPoly(Poly *poly, Poly **pFront, Poly **pBack);
    PolygonPlane ClassifyPoly(Poly *poly);
    bool IsLast();
    void SetNext(Poly *poly);
    bool operator == (const Poly &arg_) const;
    i32 GetNumberOfPolysInList() const;

    Poly();
    ~Poly();
};

#endif

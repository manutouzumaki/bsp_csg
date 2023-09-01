#ifndef _BRUSH_H_
#define _BRUSH_H_

struct IBrush
{
    Vec3 min, max;
    IBrush *m_pNext;
    Poly *m_pPolys;

    IBrush *GetNext() const {return m_pNext;}
    Poly *GetPolys() {return m_pPolys;}
    IBrush *CopyList();
    void SetNext(IBrush *pBrush_);
    void AddPoly(Poly *pPoly_);
    void AddBrush ( IBrush *pBrush_ );

    Poly *MergeList();
    void ClipToBrush(IBrush *pBrush_, bool bClipOnPlane_);
    void CalculateAABB();

    u32 GetNumberOfPolys() const;
    u32 GetNumberOfBrushes() const;

    bool IsLast() const;
    bool AABBIntersect(IBrush *pBrush_);

    IBrush();
    ~IBrush();

};

#endif

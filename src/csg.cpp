//
// CSGPoly
//

bool PlaneGetIntersection(const Plane &plane,
                           const Vec3 &start, const Vec3 &end,
                           Vec3 &intersection, f32 &percentage )
{
    Vec3 direction = end - start;
    f32	num, denom;

    Vec3Normalize(&direction);

    denom = Vec3Dot(plane.n, direction);

    if ( fabs ( denom ) < VEC_EPSILON )
    {
        return false;
    }

    num				= -(Vec3Dot(plane.n, start) + plane.d);
    percentage		= num / denom;
    intersection	= start + ( direction * percentage );
    percentage		= percentage / Vec3Len( end - start );

    return true;
}

static bool CSGPolyIsLast(CSGPoly *poly)
{
    if(poly->next == 0)
    {
        return true;
    }
    return false;
}

static void CSGPolyAddVertex(CSGPoly *poly, Vertex vertex, Arena *arena)
{
    Vertex *newVerts = (Vertex *)ArenaPushArray(arena, poly->numberOfVertices + 1, Vertex);
    memcpy(newVerts, poly->verts, sizeof(Vertex) * poly->numberOfVertices);
    poly->verts = newVerts;
    poly->verts[poly->numberOfVertices] = vertex;
    poly->numberOfVertices++;
}


static void CSGPolyAddPoly(CSGPoly *poly, CSGPoly *other)
{
    if(other != 0)
    {
        if(CSGPolyIsLast(poly))
        {
            poly->next = other;
            return;
        }
        CSGPoly *next = poly->next;
        while(!CSGPolyIsLast(next))
        {
            next = next->next;
        }
        next->next = other;
    }
}


static bool CSGPolyCalculatePlane(CSGPoly *poly)
{
    Vec3 centerOfMass;
    f32	magnitude;
    i32 i, j;

    if ( poly->numberOfVertices < 3 )
	{
		printf("Polygon has less than 3 vertices!\n");
        ASSERT(!"INVALID_CODE_PATH");
		return false;
	}

    poly->plane.n.x		= 0.0f;
    poly->plane.n.y		= 0.0f;
    poly->plane.n.z		= 0.0f;
    centerOfMass.x	= 0.0f; 
    centerOfMass.y	= 0.0f; 
    centerOfMass.z	= 0.0f;

    for ( i = 0; i < poly->numberOfVertices; i++ )
    {
        j = i + 1;

        if ( j >= poly->numberOfVertices )
		{
			j = 0;
		}

        Vertex *verts = poly->verts;

        poly->plane.n.x += ( verts[ i ].position.y - verts[ j ].position.y ) * ( verts[ i ].position.z + verts[ j ].position.z );
        poly->plane.n.y += ( verts[ i ].position.z - verts[ j ].position.z ) * ( verts[ i ].position.x + verts[ j ].position.x );
        poly->plane.n.z += ( verts[ i ].position.x - verts[ j ].position.x ) * ( verts[ i ].position.y + verts[ j ].position.y );

        centerOfMass.x += verts[ i ].position.x;
        centerOfMass.y += verts[ i ].position.y;
        centerOfMass.z += verts[ i ].position.z;
    }

    if ( ( fabs ( poly->plane.n.x ) < SMALL_EPSILON ) && ( fabs ( poly->plane.n.y ) < SMALL_EPSILON ) &&
		 ( fabs ( poly->plane.n.z ) < SMALL_EPSILON ) )
    {
         ASSERT(!"INVALID_CODE_PATH");
        return false;
    }

    magnitude = sqrt ( poly->plane.n.x * poly->plane.n.x + poly->plane.n.y * poly->plane.n.y + poly->plane.n.z * poly->plane.n.z );

    if ( magnitude < SMALL_EPSILON )
	{
        ASSERT(!"INVALID_CODE_PATH");
		return false;
	}

    poly->plane.n.x /= magnitude;
    poly->plane.n.y /= magnitude;
    poly->plane.n.z /= magnitude;

    poly->plane.n = Vec3Normalized(poly->plane.n);

    centerOfMass.x /= (f32)poly->numberOfVertices;
    centerOfMass.y /= (f32)poly->numberOfVertices;
    centerOfMass.z /= (f32)poly->numberOfVertices;

    poly->plane.d = -Vec3Dot(centerOfMass, poly->plane.n);

    return true;
}

static CSGPoly *CSGPolyCopyPoly(CSGPoly *poly, Arena *arena)
{
    CSGPoly *newPoly = (CSGPoly *)ArenaPushStruct(arena, CSGPoly);
    newPoly->numberOfVertices = poly->numberOfVertices;
    newPoly->plane = poly->plane;
    newPoly->verts = (Vertex *)ArenaPushArray(arena, newPoly->numberOfVertices, Vertex);
    newPoly->next = 0;
    memcpy(newPoly->verts, poly->verts, sizeof(Vertex) * poly->numberOfVertices);
    return newPoly;
}


static CSGPoly *CSGPolyCopyPolyList(CSGPoly *poly, Arena *arena)
{
    CSGPoly *newPoly = (CSGPoly *)ArenaPushStruct(arena, CSGPoly);
    newPoly->numberOfVertices = poly->numberOfVertices;
    newPoly->plane = poly->plane;
    newPoly->verts = (Vertex *)ArenaPushArray(arena, newPoly->numberOfVertices, Vertex);
    newPoly->next = 0;
    memcpy(newPoly->verts, poly->verts, sizeof(Vertex) * newPoly->numberOfVertices);

    if(!CSGPolyIsLast(poly))
    {
        CSGPolyAddPoly(newPoly, CSGPolyCopyPolyList(poly->next, arena));
    }

    return newPoly;
}

static ClassifyPoint CSGClassifyPoint(Vec3 v, Plane plane)
{
    f32 distance = Vec3Dot(plane.n, v) + plane.d;
    if(distance > EPSILON)
    {
        return POINT_IN_FRONT_OF_PLANE;
    }
    else if(distance < -EPSILON)
    {
        return POINT_BEHIND_PLANE;
    }
    return POINT_ON_PLANE;
}

static ClassifyPolygon CSGClassifyPoly(CSGPoly *poly, Plane plane)
{
    bool front = false, back = false;

    for(i32 i = 0; i < poly->numberOfVertices; ++i)
    {
        f32 distance = Vec3Dot(plane.n, poly->verts[i].position) + plane.d;
        if(distance > EPSILON)
        {
            if(back) return POLYGON_STRADDLING_PLANE;
            front = true;
        }
        else if(distance < -EPSILON)
        {
            if(front) return POLYGON_STRADDLING_PLANE;
            back = true;
        }
    }

    if(front)
    {
        return POLYGON_IN_FRONT_OF_PLANE;
    }
    else if(back)
    {
        return POLYGON_BEHIND_PLANE;
    }

    return POLYGON_COPLANAR_WITH_PLANE;
}

bool operator == (const CSGPoly &a, const CSGPoly &b)
{
    if(a.numberOfVertices == b.numberOfVertices)
    {
        if(a.plane.d == b.plane.d)
        {
            if(a.plane.n == b.plane.n)
            {
                for(i32 i = 0; i < a.numberOfVertices; ++i)
                {
                    if(a.verts[i].position != b.verts[i].position)
                    {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

static void CSGPolySplit(CSGPoly *poly, Plane plane, CSGPoly **ppFront, CSGPoly **ppBack, Arena *arena)
{
    ClassifyPoint *classifyPoint = (ClassifyPoint *)malloc(poly->numberOfVertices * sizeof(ClassifyPoint));

    // Classify all point
    for(i32 i = 0; i < poly->numberOfVertices; ++i)
    {
        classifyPoint[i] = CSGClassifyPoint(poly->verts[i].position, plane);
    }
    
    // Build fragments
    CSGPoly *pFront = (CSGPoly *)ArenaPushStruct(arena, CSGPoly);
    CSGPoly *pBack = (CSGPoly *)ArenaPushStruct(arena, CSGPoly);

    for(i32 i = 0; i < poly->numberOfVertices; ++i)
    {
        switch(classifyPoint[i])
        {
            case POINT_IN_FRONT_OF_PLANE:
            {
                CSGPolyAddVertex(pFront, poly->verts[i], arena);
            } break;
            case POINT_BEHIND_PLANE:
            {
                CSGPolyAddVertex(pBack, poly->verts[i], arena);
            } break;
            case POINT_ON_PLANE:
            {
                CSGPolyAddVertex(pFront, poly->verts[i], arena);
                CSGPolyAddVertex(pBack, poly->verts[i], arena);
            } break;
        }

        // check is edges should be split
        i32 iNext = i + 1;
        bool ignore = false;

        if(i == (poly->numberOfVertices - 1))
        {
            iNext = 0;
        }

        if((classifyPoint[i] == POINT_ON_PLANE) && (classifyPoint[iNext] != POINT_ON_PLANE))
        {
            ignore = true;
        }
        else if((classifyPoint[iNext] == POINT_ON_PLANE) && (classifyPoint[i] != POINT_ON_PLANE))
        {
            ignore = true;
        }

        if((!ignore) && (classifyPoint[i] != classifyPoint[iNext]))
        {
			Vertex	v;	// New vertex created by splitting
			f32	p;	// Percentage between the two points

            PlaneGetIntersection(plane, poly->verts[i].position,
                                 poly->verts[iNext].position, v.position, p);

            f32 r = InvLerp(0, 20, rand()%20);
            f32 g = InvLerp(0, 20, rand()%20);
            f32 b = InvLerp(0, 20, rand()%20);
            v.color = {r, g, b, 1.0f};
            
            CSGPolyAddVertex(pFront, v, arena);
            CSGPolyAddVertex(pBack, v, arena);
        } 
    }

    free(classifyPoint);

    ASSERT(pFront->numberOfVertices >= 3);
    ASSERT(pBack->numberOfVertices >= 3);

    CSGPolyCalculatePlane(pFront);
    CSGPolyCalculatePlane(pBack);

    *ppFront = pFront;
    *ppBack = pBack;
}

static CSGPoly *CSGPolyClip(CSGPoly *pThis, CSGPoly *poly, bool clipOnPlane, Arena *arena)
{
    switch(CSGClassifyPoly(poly, pThis->plane))
    {
        case POLYGON_IN_FRONT_OF_PLANE:
        {
            return CSGPolyCopyPoly(poly, arena);
        } break;
        case POLYGON_BEHIND_PLANE:
        {
            if(CSGPolyIsLast(pThis)) return 0;
            return CSGPolyClip(pThis->next, poly, clipOnPlane, arena);
        } break;
        case POLYGON_COPLANAR_WITH_PLANE:
        {
            f32 angle = Vec3Dot(pThis->plane.n, poly->plane.n) - 1;
            if((angle < VEC_EPSILON) && (angle > -VEC_EPSILON))
            {
                if(!clipOnPlane)
                {
                    return CSGPolyCopyPoly(poly, arena);
                }
            }
            if(CSGPolyIsLast(pThis)) return 0;
            return CSGPolyClip(pThis->next, poly, clipOnPlane, arena);
        } break;
        case POLYGON_STRADDLING_PLANE:
        {
            CSGPoly *pFront = 0;
            CSGPoly *pBack  = 0;
            CSGPolySplit(poly, pThis->plane, &pFront, &pBack, arena);
            if(CSGPolyIsLast(pThis)) return pFront;
            CSGPoly *pBackFrags = CSGPolyClip(pThis->next, pBack, clipOnPlane, arena);
            if(pBackFrags == 0) return pFront;
            if(*pBackFrags == *pBack) return CSGPolyCopyPoly(poly, arena);
            CSGPolyAddPoly(pFront, pBackFrags);
            return pFront;
        } break;
    }

    return 0;
}



static void CSGPolySetNext(CSGPoly *poly, CSGPoly *next)
{
    if(CSGPolyIsLast(poly))
    {
        poly->next = next;
        return;
    }
    
    CSGPoly *pPoly = next;
    while(!CSGPolyIsLast(pPoly))
    {
        pPoly = pPoly->next;
    }
    CSGPolySetNext(pPoly, poly->next);
    poly->next = next;
}

static i32 CSGPolyListCount(CSGPoly *poly)
{
    i32 count = 0;
    for(CSGPoly *p = poly; p; p = p->next)
    {
        count++;
    }
    return count;
}

//
// CSGBrush
//

static bool CSGBrushIsLast(CSGBrush *brush)
{
    if(brush->next == 0)
    {
        return true;
    }
    return false;
}

bool CSGBrushAABBIntersect(CSGBrush *a, CSGBrush *b)
{
    if((a->min.x > b->max.x) || (b->min.x > a->max.x)) return false; 
    if((a->min.y > b->max.y) || (b->min.y > a->max.y)) return false; 
    if((a->min.z > b->max.z) || (b->min.z > a->max.z)) return false; 
    return true;
}


void CSGBrushSetNext(CSGBrush *brush, CSGBrush *next)
{
    if(CSGBrushIsLast(brush))
    {
        brush->next = next;
        return;
    }

    if(next == 0) brush->next = 0;
    else
    {
        CSGBrush *pBrush = next;
        while(!CSGBrushIsLast(pBrush))
        {
            pBrush = pBrush->next;
        }
        CSGBrushSetNext(pBrush, brush->next);
        brush->next = pBrush;
    }
}

CSGBrush *CSGBrushCopyList(CSGBrush *brush, Arena *arena)
{
    CSGBrush *newBrush = (CSGBrush *)ArenaPushStruct(arena, CSGBrush);
    newBrush->min = brush->min;
    newBrush->max = brush->max;
    newBrush->polys = CSGPolyCopyPolyList(brush->polys, arena);
    if(!CSGBrushIsLast(brush))
    {
        CSGBrushSetNext(newBrush, CSGBrushCopyList(brush->next, arena));
    }
    return newBrush;
}

void CSGBrushAddPoly(CSGBrush *brush, CSGPoly *poly)
{
    if(brush->polys == 0)
    {
        brush->polys = poly;
        return;
    }

    CSGPoly *pPoly = brush->polys;
    while(!CSGPolyIsLast(pPoly))
    {
        pPoly = pPoly->next;
    }
    CSGPolySetNext(pPoly, poly);
}

void CSGBrushAddBrush(CSGBrush *brush, CSGBrush *other)
{
    if(other != 0)
    {
        if(CSGBrushIsLast(brush))
        {
            brush->next = other;
            return;
        }
        CSGBrush *pBrush =  brush->next;
        while(!CSGBrushIsLast(pBrush))
        {
            pBrush = pBrush->next;
        }
        pBrush->next =  other;
    }
}

i32 CSGBrushGetNumberOfPolys(CSGBrush *brush)
{
    CSGPoly *poly = brush->polys;
    i32 count = 0;
    while(poly != 0)
    {
        poly = poly->next;
        count++;
    }
    return count;
}

i32 CSGBrushGetNumberOfBrushes(CSGBrush *brush)
{
    CSGBrush *pBrush = brush->next;
    i32 count = 1;
    while(pBrush != 0)
    {
        pBrush = pBrush->next;
        count++;
    }
    return count;
}

void CSGBrushCalculateAABB(CSGBrush *brush)
{
    brush->min = brush->polys->verts[0].position;
	brush->max = brush->polys->verts[0].position;

	CSGPoly *pPoly = brush->polys;

	for (i32 i = 0; i < CSGBrushGetNumberOfPolys(brush); ++i)
	{
		for (i32 j = 0; j < pPoly->numberOfVertices; ++j)
		{
			//
			// Calculate min
			//
			if (pPoly->verts[ j ].position.x < brush->min.x)
			{
				brush->min.x = pPoly->verts[ j ].position.x;
			}

			if (pPoly->verts[ j ].position.y < brush->min.y)
			{
				brush->min.y = pPoly->verts[ j ].position.y;
			}

			if (pPoly->verts[ j ].position.z < brush->min.z)
			{
				brush->min.z = pPoly->verts[ j ].position.z;
			}

			//
			// Calculate max
			//
			if (pPoly->verts[ j ].position.x > brush->max.x)
			{
				brush->max.x = pPoly->verts[ j ].position.x;
			}

			if (pPoly->verts[ j ].position.y > brush->max.y)
			{
				brush->max.y = pPoly->verts[ j ].position.y;
			}

			if (pPoly->verts[ j ].position.z > brush->max.z)
			{
				brush->max.z = pPoly->verts[ j ].position.z;
			}
		}

		pPoly = pPoly->next;
	}

}

void CSGBrushClip(CSGBrush *pThis, CSGBrush *brush, bool clipOnPlane, Arena *arena)
{
    CSGPoly *pPolyList = 0;
    CSGPoly *pPoly = pThis->polys;
    for(i32 i = 0; i < CSGBrushGetNumberOfPolys(pThis); ++i)
    {
        CSGPoly *pClippedPoly = CSGPolyClip(brush->polys, pPoly, clipOnPlane, arena);
        if(pPolyList == 0)
        {
            pPolyList = pClippedPoly;
        }
        else
        {
            CSGPolyAddPoly(pPolyList, pClippedPoly);
        }
        pPoly = pPoly->next;
    }
    pThis->polys = pPolyList;
}

CSGPoly *MergeList(CSGBrush *pThis, Arena *arena)
{
    CSGBrush *pClippedList = CSGBrushCopyList(pThis, arena);
    CSGBrush *pClip = pClippedList;
    CSGBrush *pBrush = 0;
    CSGPoly  *pPolyList = 0;
    bool clipOnPlane = false;
    u32 uiBrushes = CSGBrushGetNumberOfBrushes(pThis);
    for(i32 i = 0; i < uiBrushes; ++i)
    {
        pBrush = pThis;
        clipOnPlane = false;

        for(i32 j = 0; j < uiBrushes; ++j)
        {
            if(i == j)
            {
                clipOnPlane = true;
            }
            else
            {
                if(CSGBrushAABBIntersect(pClip, pBrush))
                {
                    CSGBrushClip(pClip, pBrush, clipOnPlane, arena);
                }
            }
            pBrush = pBrush->next;
        }
        pClip = pClip->next;
    }

    pClip = pClippedList;

    while(pClip != 0)
    {
        if(CSGBrushGetNumberOfPolys(pClip) != 0)
        {
            // Extract brushes left over polygons and add them to the list
            CSGPoly *pPoly = CSGPolyCopyPolyList(pClip->polys, arena);
            if(pPolyList == 0)
            {
                pPolyList = pPoly;
            }
            else
            {
                CSGPolyAddPoly(pPolyList, pPoly);
            }
            pClip = pClip->next;
        }
        else
        {
            // Brush has no polygons and should be deleted
            if(pClip == pClippedList)
            {
                pClip = pClippedList->next;
                CSGBrushSetNext(pClippedList, 0);
                pClippedList = pClip;
            }
            else
            {
                CSGBrush *pTemp = pClippedList;
                while(pTemp != 0)
                {
                    if(pTemp->next == pClip)
                    {
                        break;
                    }
                    pTemp = pTemp->next;
                }
                pTemp->next = pClip->next;
                CSGBrushSetNext(pClip, 0);
                pClip = pTemp->next;
            }
        }
    }
    return pPolyList;
}



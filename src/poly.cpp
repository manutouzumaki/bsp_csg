PolygonPlane ClassifyPoint(const Plane &plane, const Vec3 &v)
{
    f32 distance = Vec3Dot(plane.n, v) + plane.d;
    if(distance > SMALL_EPSILON)
    {
        return FRONT;
    }
    else if(distance < -SMALL_EPSILON)
    {
        return BACK;
    }
    return ONPLANE;
}

bool PlaneGetIntersection(const Plane &plane,
                           const Vec3 &Start, const Vec3 &End,
                           Vec3 &Intersection, f32 &Percentage )
{
    Vec3	Direction = End - Start;
    f32	Num, Denom;

    Vec3Normalize(&Direction);

    Denom = Vec3Dot(plane.n, Direction);

    if ( fabs ( Denom ) < VEC_EPSILON )
    {
        return false;
    }

    Num				= -(Vec3Dot(plane.n, Start) + plane.d);
    Percentage		= Num / Denom;
    Intersection	= Start + ( Direction * Percentage );
    Percentage		= Percentage / Vec3Len( End - Start );

    return true;
}


i32 Poly::GetNumberOfPolysInList() const
{
    i32 count = 0;
    for(Poly *polygon = (Poly *)this; polygon; polygon = polygon->next)
    {
        count++;
    }
    return count;
}

void Poly::SplitPoly(Poly *pPoly_, Poly **ppFront, Poly **ppBack)
{
    PolygonPlane *pCP = new PolygonPlane[pPoly_->numberOfVertices];

    // Classify all point
    for(i32 i = 0; i < pPoly_->numberOfVertices; i++)
    {
        pCP[i] = ClassifyPoint(plane, pPoly_->verts[i].position); 
    }

    // Build fragments
    Poly *pFront = new Poly;
    Poly *pBack = new Poly;

    pFront->plane = pPoly_->plane;
    pBack->plane  = pPoly_->plane;

    for(i32 i = 0; i < pPoly_->numberOfVertices; i++)
    {
        // Add point to appropriate list
        switch(pCP[i])
        {
            case FRONT:
            {
                pFront->AddVertex(pPoly_->verts[i]);
            }break;
            case BACK:
            {
                pBack->AddVertex(pPoly_->verts[i]);
            } break;
            case ONPLANE:
            {
                pFront->AddVertex(pPoly_->verts[i]);
                pBack->AddVertex(pPoly_->verts[i]);
            } break;
        }

        // check is edges should be split
        i32 iNext = i + 1;
        bool bIgnore = false;

        if(i == (pPoly_->numberOfVertices - 1))
        {
            iNext = 0;
        }

        if((pCP[i] == ONPLANE) && (pCP[iNext] != ONPLANE))
        {
            bIgnore = true;
        }
        else if((pCP[iNext] == ONPLANE) && (pCP[i] != ONPLANE))
        {
            bIgnore = true;
        }

        if((!bIgnore) && (pCP[i] != pCP[iNext]))
        {
			Vertex	v;	// New vertex created by splitting
			f32	p;	// Percentage between the two points

            PlaneGetIntersection(plane, pPoly_->verts[i].position,
                                 pPoly_->verts[iNext].position, v.position, p);

            f32 r = InvLerp(0, 20, rand()%20);
            f32 g = InvLerp(0, 20, rand()%20);
            f32 b = InvLerp(0, 20, rand()%20);
            v.color = {r, g, b, 1.0f};

			pFront->AddVertex ( v );
			pBack->AddVertex ( v );
        }
    }

    delete [] pCP;

    ASSERT(pFront->numberOfVertices >= 3);
    ASSERT(pBack->numberOfVertices >= 3);

    pFront->CalculatePlane();
    pBack->CalculatePlane();
    *ppFront = pFront;
    *ppBack = pBack;

}

PolygonPlane Poly::ClassifyPoly(Poly *poly)
{
    bool bFront = false, bBack = false;
    f32 dist;
    for(i32 i = 0; i < (i32)poly->numberOfVertices; i++)
    {
        dist = Vec3Dot(plane.n, poly->verts[i].position) + plane.d;
		if ( dist > EPSILON )
		{
			if ( bBack )
			{
				return SPLIT;
			}

			bFront = true;
		}
		else if ( dist < -EPSILON )
		{
			if ( bFront )
			{
				return SPLIT;
			}

			bBack = true;
		}
    }

	if ( bFront )
	{
		return FRONT;
	}
	else if ( bBack )
	{
		return BACK;
	}

	return ONPLANE;
}


Poly *Poly::ClipToList(Poly *pPoly_, bool bClipOnPlane_)
{
 	switch ( ClassifyPoly ( pPoly_ ) )
	{
	case FRONT:
		{
			return pPoly_->CopyPoly ( );
		} break;

	case BACK:
		{
			if ( IsLast ( ) )
			{
				return NULL;
			}

			return next->ClipToList ( pPoly_, bClipOnPlane_ );
		} break;

	case ONPLANE:
		{
			f32	Angle = Vec3Dot(plane.n, pPoly_->plane.n) - 1;

			if ( ( Angle < VEC_EPSILON ) && ( Angle > -VEC_EPSILON ) )
			{
				if ( !bClipOnPlane_ )
				{
					return pPoly_->CopyPoly ( );
				}
			}

			if ( IsLast ( ) )
			{
				return NULL;
			}

			return next->ClipToList ( pPoly_, bClipOnPlane_ );
		} break;

	case SPLIT:
		{
			Poly *pFront	= NULL;
			Poly *pBack		= NULL;

			SplitPoly ( pPoly_, &pFront, &pBack );

			if ( IsLast ( ) )
			{
				delete pBack;

				return pFront;
			}

			Poly *pBackFrags = next->ClipToList ( pBack, bClipOnPlane_ );

			if ( pBackFrags == NULL )
			{
				delete pBack;

				return pFront;
			}

			if ( *pBackFrags == *pBack )
			{
				delete pFront;
				delete pBack;
				delete pBackFrags;

				return pPoly_->CopyPoly ( );
			}

			delete pBack;

			pFront->AddPoly ( pBackFrags );

			return pFront;
		} break;
	}

	return NULL;
}

Poly *Poly::CopyList()
{
    Poly *pPoly = new Poly;
    pPoly->numberOfVertices = numberOfVertices;
    pPoly->plane = plane;
    pPoly->verts = new Vertex[numberOfVertices];
    memcpy(pPoly->verts, verts, sizeof(Vertex) * numberOfVertices);

    if(!IsLast())
    {
        pPoly->AddPoly(next->CopyList());
    }

    return pPoly;
}

Poly *Poly::CopyPoly()
{
    Poly *pPoly = new Poly;
    pPoly->numberOfVertices = numberOfVertices;
    pPoly->plane = plane;
    pPoly->verts = new Vertex[numberOfVertices];
    memcpy(pPoly->verts, verts, sizeof(Vertex) * numberOfVertices);
    return pPoly;
}

void Poly::AddVertex(Vertex &vertex)
{
    Vertex *pVertices = new Vertex[numberOfVertices + 1];
    memcpy(pVertices, verts, sizeof(Vertex) * numberOfVertices);
    delete [] verts;
    verts = pVertices;
    verts[numberOfVertices] = vertex;
    numberOfVertices++;
}

void Poly::AddPoly(Poly *poly)
{
    if(poly != 0)
    {
        if(IsLast())
        {
            next = poly;
            return;
        }

        Poly *pPoly = next;
        while(!pPoly->IsLast())
        {
            pPoly = pPoly->next;
        }
        pPoly->next = poly;
    }
}

bool Poly::CalculatePlane()
{
    Vec3 centerOfMass;
    f32	magnitude;
    i32 i, j;

    if ( numberOfVertices < 3 )
	{
		printf("Polygon has less than 3 vertices!\n");
        ASSERT(!"INVALID_CODE_PATH");
		return false;
	}

    plane.n.x		= 0.0f;
    plane.n.y		= 0.0f;
    plane.n.z		= 0.0f;
    centerOfMass.x	= 0.0f; 
    centerOfMass.y	= 0.0f; 
    centerOfMass.z	= 0.0f;

    for ( i = 0; i < numberOfVertices; i++ )
    {
        j = i + 1;

        if ( j >= numberOfVertices )
		{
			j = 0;
		}

        plane.n.x += ( verts[ i ].position.y - verts[ j ].position.y ) * ( verts[ i ].position.z + verts[ j ].position.z );
        plane.n.y += ( verts[ i ].position.z - verts[ j ].position.z ) * ( verts[ i ].position.x + verts[ j ].position.x );
        plane.n.z += ( verts[ i ].position.x - verts[ j ].position.x ) * ( verts[ i ].position.y + verts[ j ].position.y );

        centerOfMass.x += verts[ i ].position.x;
        centerOfMass.y += verts[ i ].position.y;
        centerOfMass.z += verts[ i ].position.z;
    }

    if ( ( fabs ( plane.n.x ) < SMALL_EPSILON ) && ( fabs ( plane.n.y ) < SMALL_EPSILON ) &&
		 ( fabs ( plane.n.z ) < SMALL_EPSILON ) )
    {
        ASSERT(!"INVALID_CODE_PATH");
        return false;
    }

    magnitude = sqrt ( plane.n.x * plane.n.x + plane.n.y * plane.n.y + plane.n.z * plane.n.z );

    if ( magnitude < SMALL_EPSILON )
	{
        ASSERT(!"INVALID_CODE_PATH");
		return false;
	}

    plane.n.x /= magnitude;
    plane.n.y /= magnitude;
    plane.n.z /= magnitude;

    plane.n = Vec3Normalized(plane.n);

    centerOfMass.x /= (f32)numberOfVertices;
    centerOfMass.y /= (f32)numberOfVertices;
    centerOfMass.z /= (f32)numberOfVertices;

    plane.d = -Vec3Dot(centerOfMass, plane.n);

    return true;

}

bool Poly::IsLast()
{
    if(next == 0)
    {
        return true;
    }
    return false;
}

bool Poly::operator == (const Poly &arg_) const
{
    if(numberOfVertices == arg_.numberOfVertices)
    {
        if(plane.d == arg_.plane.d)
        {
            if(plane.n == plane.n)
            {
                for(i32 i = 0; i < numberOfVertices; ++i)
                {
                    if(verts[i].position != arg_.verts[i].position)
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


void Poly::SetNext(Poly *poly)
{
    if(IsLast())
    {
        next = poly;
        return;
    }

    Poly *pPoly = poly;
    while(!pPoly->IsLast())
    {
        pPoly = pPoly->next;
    }
    pPoly->SetNext(next);
    next = poly;
}

Poly::Poly()
{
    next = 0;
    verts = 0;
    numberOfVertices = 0;
}

Poly::~Poly()
{
    if(!IsLast())
    {
        delete next;
        next = 0;
    }

    if(verts != 0)
    {
        delete [] verts;
        verts = 0;
        numberOfVertices = 0;
    }
}

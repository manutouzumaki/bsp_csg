IBrush *IBrush::CopyList()
{
	IBrush *pBrush = new IBrush;

	pBrush->max = max;
	pBrush->min = min;

	pBrush->m_pPolys = m_pPolys->CopyList ( );

	if ( !IsLast ( ) )
	{
		pBrush->SetNext ( m_pNext->CopyList ( ) );
	}

	return pBrush;
}

void IBrush::SetNext(IBrush *pBrush_)
{
	if ( IsLast ( ) )
	{
		m_pNext = pBrush_;

		return;
	}

	if ( pBrush_ == 0 )
	{
		m_pNext = 0;
	}
	else
	{
		IBrush *pBrush = pBrush_;

		while ( !pBrush->IsLast ( ) )
		{
			pBrush = pBrush->GetNext ( );
		}

		pBrush->SetNext ( m_pNext );

		m_pNext = pBrush_;
	}
}

void IBrush::AddPoly(Poly *pPoly_)
{
	if ( m_pPolys == 0 )
	{
		m_pPolys = pPoly_;

		return;
	}

	Poly *pPoly = m_pPolys;

	while ( !pPoly->IsLast ( ) )
	{
		pPoly = pPoly->next;
	}

	pPoly->SetNext ( pPoly_ );
}

Poly *IBrush::MergeList()
{
	IBrush			*pClippedList	= CopyList ( );
	IBrush			*pClip			= pClippedList;
	IBrush			*pBrush			= NULL;
	Poly			*pPolyList		= NULL;

	bool			bClipOnPlane	= false;
	unsigned int	uiBrushes		= GetNumberOfBrushes ( );

	for ( int i = 0; i < uiBrushes; i++ )
	{
		pBrush			= this;
		bClipOnPlane	= false;

		for ( int j = 0; j < uiBrushes; j++ )
		{
			if ( i == j )
			{
				bClipOnPlane = true;
			}
			else
			{
				if ( pClip->AABBIntersect ( pBrush ) )
				{
					pClip->ClipToBrush ( pBrush, bClipOnPlane );
				}
			}

			pBrush = pBrush->GetNext ( );
		}

		pClip = pClip->GetNext ( );
	}

	pClip = pClippedList;

	while ( pClip != NULL )
	{
		if ( pClip->GetNumberOfPolys ( ) != 0 )
		{
			//
			// Extract brushes left over polygons and add them to the list
			//
			Poly *pPoly = pClip->GetPolys ( )->CopyList ( );

			if ( pPolyList == NULL )
			{
				pPolyList = pPoly;
			}
			else
			{
				pPolyList->AddPoly ( pPoly );
			}

			pClip = pClip->GetNext ( );
		}
		else
		{
			//
			// Brush has no polygons and should be deleted
			//
			if ( pClip == pClippedList )
			{
				pClip = pClippedList->GetNext ( );

				pClippedList->SetNext ( NULL );

				delete pClippedList;

				pClippedList = pClip;
			}
			else
			{
				IBrush	*pTemp = pClippedList;

				while ( pTemp != NULL )
				{
					if ( pTemp->GetNext ( ) == pClip )
					{
						break;
					}

					pTemp = pTemp->GetNext ( );
				}

				pTemp->m_pNext = pClip->GetNext ( );
				pClip->SetNext ( NULL );

				delete pClip;

				pClip = pTemp->GetNext ( );
			}
		}
	}

	delete pClippedList;

	return pPolyList;
}

void IBrush::ClipToBrush(IBrush *pBrush_, bool bClipOnPlane_)
{
	Poly *pPolyList = NULL;
	Poly *pPoly		= m_pPolys;

	for ( int i = 0; i < GetNumberOfPolys ( ); i++ )
	{
		Poly *pClippedPoly = pBrush_->GetPolys ( )->ClipToList ( pPoly, bClipOnPlane_ );

		if ( pPolyList == NULL )
		{
			pPolyList = pClippedPoly;
		}
		else
		{
			pPolyList->AddPoly ( pClippedPoly );
		}

		pPoly = pPoly->next;
	}

	delete m_pPolys;
	m_pPolys = pPolyList;
}

void IBrush::CalculateAABB()
{
	min = m_pPolys->verts[ 0 ].position;
	max = m_pPolys->verts[ 0 ].position;

	Poly *pPoly = m_pPolys;

	for ( int i = 0; i < GetNumberOfPolys ( ); i++ )
	{
		for ( int j = 0; j < pPoly->numberOfVertices; j++ )
		{
			//
			// Calculate min
			//
			if ( pPoly->verts[ j ].position.x < min.x )
			{
				min.x = pPoly->verts[ j ].position.x;
			}

			if ( pPoly->verts[ j ].position.y < min.y )
			{
				min.y = pPoly->verts[ j ].position.y;
			}

			if ( pPoly->verts[ j ].position.z < min.z )
			{
				min.z = pPoly->verts[ j ].position.z;
			}

			//
			// Calculate max
			//
			if ( pPoly->verts[ j ].position.x > max.x )
			{
				max.x = pPoly->verts[ j ].position.x;
			}

			if ( pPoly->verts[ j ].position.y > max.y )
			{
				max.y = pPoly->verts[ j ].position.y;
			}

			if ( pPoly->verts[ j ].position.z > max.z )
			{
				max.z = pPoly->verts[ j ].position.z;
			}
		}

		pPoly = pPoly->next;
	}
}

u32 IBrush::GetNumberOfPolys() const
{
	Poly			*pPoly		= m_pPolys;
	unsigned int	uiCount		= 0;

	while ( pPoly != NULL )
	{
		pPoly = pPoly->next;
		uiCount++;
	}

	return uiCount;
}

u32 IBrush::GetNumberOfBrushes() const
{
	IBrush			*pBrush		= m_pNext;
	unsigned int	uiCount		= 1;

	while ( pBrush != NULL )
	{
		pBrush = pBrush->GetNext ( );
		uiCount++;
	}

	return uiCount;
}

bool IBrush::IsLast() const
{
	if ( m_pNext == 0 )
	{
		return true;
	}

	return false;
}

bool IBrush::AABBIntersect(IBrush *pBrush_)
{
	if ( ( min.x > pBrush_->max.x ) || ( pBrush_->min.x > max.x ) )
	{
		return false;
	}

	if ( ( min.y > pBrush_->max.y ) || ( pBrush_->min.y > max.y ) )
	{
		return false;
	}

	if ( ( min.z > pBrush_->max.z ) || ( pBrush_->min.z > max.z ) )
	{
		return false;
	}

	return true;
}

void IBrush::AddBrush ( IBrush *pBrush_ )
{
	if ( pBrush_ != 0 )
	{
		if ( IsLast ( ) )
		{
			m_pNext = pBrush_;

			return;
		}

		IBrush *pBrush = m_pNext;

		while ( !pBrush->IsLast ( ) )
		{
			pBrush = pBrush->GetNext ( );
		}

		pBrush->m_pNext = pBrush_;
	}
}

IBrush::IBrush()
{
    m_pNext = 0;
    m_pPolys = 0;
}

IBrush::~IBrush()
{
	if ( m_pPolys != 0 )
	{
		delete m_pPolys;
		m_pPolys = 0;
	}

	if ( !IsLast ( ) )
	{
		delete m_pNext;
		m_pNext = 0;
	}
}

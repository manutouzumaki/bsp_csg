static int ClassifyPointToPlane(Vec3 p, Plane plane)
{
    // Compute signed distance of point from plane
    f32 dist = Vec3Dot(plane.n, p) + plane.d;
    // Classify p based on the signed distance
    if (dist > VEC_EPSILON)
        return POINT_IN_FRONT_OF_PLANE;
    if (dist < -VEC_EPSILON)
        return POINT_BEHIND_PLANE;
    return POINT_ON_PLANE;
}

static int ClassifyPolygonToPlane(CSGPoly *poly, Plane plane)
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

static Plane PickSplittingPlane(std::vector<CSGPoly *> &polygons, i32 &outIndex)
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
            outIndex = i;
        }
    }
    return bestPlane;
}

static void SplitPolygon(CSGPoly *poly, Plane plane, CSGPoly **frontPoly, CSGPoly **backPoly, Arena *arena)
{
    int numFront = 0, numBack = 0;
    Vec3 frontVerts[256], backVerts[256];
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
    *frontPoly = (CSGPoly *)ArenaPushStruct(arena, CSGPoly);
    *backPoly = (CSGPoly *)ArenaPushStruct(arena, CSGPoly);
    for(i32 i = 0; i < numFront; i++)
    {
        Vertex vertex;
        vertex.position = frontVerts[i];
        vertex.color = {0, 0, 0, 1};
        CSGPolyAddVertex(*frontPoly, vertex, arena);
    }
    for(i32 i = 0; i < numBack; i++)
    {
        Vertex vertex;
        vertex.position = backVerts[i];
        vertex.color = {0, 0, 0, 1};
        CSGPolyAddVertex(*backPoly, vertex, arena);
    }

    CSGPolyCalculatePlane(*frontPoly);
    CSGPolyCalculatePlane(*backPoly);
}

static BSPNode *BSPNodeCreateLeaf(BSPType type, Arena *arena)
{
    BSPNode *node = (BSPNode *)ArenaPushStruct(arena, BSPNode);
    node->plane = {};
    node->back = 0;
    node->front = 0;
    node->type = type;
    return node;
}

static BSPNode *BSPNodeCreateNode(BSPNode *front, BSPNode *back, Plane plane, Arena *arena)
{
    BSPNode *node = (BSPNode *)ArenaPushStruct(arena, BSPNode);
    node->plane = plane;
    node->front = front;
    node->back = back;
    node->type = BSP_TYPE_NODE;
    return node;
}

static bool BSPNodeIsLeaf(BSPNode *node)
{
    if(node->front == 0 && node->back == 0)
        return true;
    return false;
}

static bool BSPNodeIsSolid(BSPNode *node)
{
    return node->type == BSP_TYPE_SOLID;
}

static BSPNode *BuildBSPTree(std::vector<CSGPoly *> &polygons, BSPState state, Arena *arena, Arena *bspArena)
{
    i32 numPolygons = polygons.size();
    
    // If criterion for a leaf is matche, create a leaf node from remaining polygons
    if(numPolygons <= 0)
    {
        if(state == BSP_FRONT)
        {
            return BSPNodeCreateLeaf(BSP_TYPE_EMPTY, bspArena);
        }
        else if(state == BSP_BACK)
        {
            return BSPNodeCreateLeaf(BSP_TYPE_SOLID, bspArena);
        }
    }
    // Select best possible partitioning plane base on the input geometry

    i32 index = -1;
    Plane splitPlane = PickSplittingPlane(polygons, index);

    std::vector<CSGPoly *> frontList, backList;

    // Test each polygon against the divding plane, adding them to the front list, back list, or both
    // as appropiate
    for(i32 i = 0; i < numPolygons; i++)
    {
        // dont check with it self
        if(i == index) continue;

        CSGPoly *poly = polygons[i], *frontPart, *backPart;
        ClassifyPolygon result = (ClassifyPolygon)ClassifyPolygonToPlane(poly, splitPlane);
        switch(result)
        {
            case POLYGON_COPLANAR_WITH_PLANE:
            case POLYGON_IN_FRONT_OF_PLANE:
                frontList.push_back(poly);
                break;
            case POLYGON_BEHIND_PLANE:
                backList.push_back(poly);
                break;
            case POLYGON_STRADDLING_PLANE:
                // Split polygon to plane and send a part to each side of the plane
                SplitPolygon(poly, splitPlane, &frontPart, &backPart, arena);
                frontList.push_back(frontPart);
                backList.push_back(backPart);
                break;
        }
    }

    // Recursively build child subtrees and return new tree root combining them
    BSPNode *frontTree = BuildBSPTree(frontList, BSP_FRONT, arena, bspArena);
    BSPNode *backTree  = BuildBSPTree(backList, BSP_BACK, arena, bspArena);
    return BSPNodeCreateNode(frontTree, backTree, splitPlane, bspArena);
}

static int PointInSolidSpace(BSPNode *node, Vec3 p)
{
    if(node == 0) return POINT_IN_FRONT_OF_PLANE;

    while(!BSPNodeIsLeaf(node))
    {
        f32 dist = Vec3Dot(node->plane.n, p) + node->plane.d;
        node = node->child[dist < FLT_EPSILON];
    }
    // Now at a leaf, inside/outside status determined by solid flag
    return BSPNodeIsSolid(node) ? POINT_BEHIND_PLANE : POINT_IN_FRONT_OF_PLANE;
}

static int IntersectionLinePlane(Vec3 a, Vec3 b, Plane p, f32 &t, Vec3 &q)
{
    Vec3 ab = b - a;
    t = (-p.d - Vec3Dot(p.n, a)) / Vec3Dot(p.n, ab);
    if(t >= 0.0f && t <= 1.0f)
    {
        q = a + ab * t;
        return 1;
    }
    return 0;
}

static int RayIntersect(BSPNode *node, Vec3 a, Vec3 b, Vec3 *outpoint, Plane *outplane)
{
    if(node == 0) return 0;
    if(BSPNodeIsLeaf(node))
    {
        if(BSPNodeIsSolid(node))
        {
            if(Vec3LenSq(outplane->n) > FLT_EPSILON)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    if(node->plane.n.y == 1.0f && node->plane.d < 0 && node->plane.d > -1.06 && Vec3LenSq(b - a) > 0)
    {
        i32 StopHere = 0;
    }
    // TODO: play with this EPSILON
    i32 aPoint = (i32)(((Vec3Dot(node->plane.n, a) + node->plane.d)) < BSP_EPSILON);
    i32 bPoint = (i32)(((Vec3Dot(node->plane.n, b) + node->plane.d)) < BSP_EPSILON);
    
    if(aPoint == POINT_IN_FRONT_OF_PLANE && bPoint == POINT_IN_FRONT_OF_PLANE)
    {
        // the ray is infront of the plane we travers the front side
        return RayIntersect(node->front, a, b, outpoint, outplane);
    }
    else if(aPoint == POINT_BEHIND_PLANE && bPoint == POINT_BEHIND_PLANE)
    {
        // the ray is behind of the plane we travers the back side 
        return RayIntersect(node->back, a, b, outpoint, outplane);
    }
    else
    {
        // here the ray intersect the plane, so we check for the intersection and split the ray
        f32 t = -1.0f;
        Vec3 q;
        if(IntersectionLinePlane(a, b, node->plane, t, q))
        {
            bool frontResult, backResult;
            if(aPoint == POINT_IN_FRONT_OF_PLANE)
            {
                *outpoint = q;
                *outplane = node->plane;

                // check the front part
                frontResult = RayIntersect(node->front, a, q, outpoint, outplane);
                // check the back part
                backResult = RayIntersect(node->back, q, b, outpoint, outplane);
            }
            else
            {
                // check the back part
                frontResult = RayIntersect(node->back, a, q, outpoint, outplane);
                // check the front part
                backResult = RayIntersect(node->front, q, b, outpoint, outplane);
            }
            return frontResult || backResult;
        }
        else
        {
            return 0;
        }

    }
}

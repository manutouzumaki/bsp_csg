#ifndef _BSP_H_
#define _BSP_H_

enum BSPType
{
    BSP_TYPE_NODE,
    BSP_TYPE_SOLID,
    BSP_TYPE_EMPTY
};

struct BSPNode
{
    BSPType type;
    Plane plane;
    union
    {
        struct
        {
            BSPNode *front;
            BSPNode *back;
        };
        BSPNode *child[2];
    };
};

#endif

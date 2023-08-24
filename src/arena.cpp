Memory MemoryInit(size_t size)
{
    Memory memory;

    memory.data = (u8 *)VirtualAlloc(0, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    memory.size = size;
    memory.used = 0;

    memset(memory.data, 0, size);

    return memory;
}

void MemoryShutdown(Memory *memory)
{
    VirtualFree(memory->data, 0, MEM_RELEASE);
}

Arena ArenaCreate(Memory *memory, size_t size)
{
    Arena arena;
    ASSERT(memory->used + size <= memory->size);
    arena.base = memory->data + memory->used;
    arena.size = size;
    arena.used = 0;
    memory->used += size;
    memset(arena.base, 0, size);
    return arena;
}

void ArenaClear(Arena *arena)
{
    memset(arena->base, 0, arena->size);
    arena->used = 0;
}

void *ArenaPush(Arena *arena, size_t size)
{
    ASSERT(arena->used + size <= arena->size);
    u8 *result = arena->base + arena->used;
    arena->used += size;
    return (void *)result;
}

#define ArenaPushStruct(arena, type) ArenaPush(arena, sizeof(type))
#define ArenaPushArray(arena, count, type) ArenaPush(arena, sizeof(type)*(count))

#define ArenaTmpBegin(memory, arena) ArenaCreate(memory, arena)
void ArenaTmpEnd(Memory *memory, Arena *arena)
{
    ArenaClear(arena);
    memory->used -= arena->size;
}


/*
LinkList LinkListCreate(size_t dataSize, Arena *arena)
{
    LinkList linkList = {};
    linkList.arena = arena;
    linkList.dataSize = dataSize;
    linkList.front = 0;
    linkList.back = 0;
    linkList.free = 0;
    return linkList;
}

LinkNode *LinkListAdd(LinkList *linkList, void *data)
{
    if(linkList->front == 0 && linkList->free == 0)
    {
        // add the element to the first
        u8 *mem = (u8 *)ArenaPush(linkList->arena, linkList->dataSize * sizeof(LinkNode));
        LinkNode *node = (LinkNode *)mem;
        node->data = (void *)((u8 *)node + sizeof(LinkNode));
        memcpy(node->data, data, linkList->dataSize);
        node->next = 0;
        linkList->front = node;
        linkList->back = linkList->front;
        return node;
    }
    else if(linkList->free == 0)
    {
        u8 *mem = (u8 *)ArenaPush(linkList->arena, linkList->dataSize * sizeof(LinkNode));
        LinkNode *node = (LinkNode *)mem;
        node->data = (void *)((u8 *)node + sizeof(LinkNode));
        memcpy(node->data, data, linkList->dataSize);
        node->next = 0;
        node->prev = linkList->back;
        linkList->back->next = node;
        linkList->back = linkList->back->next;
        return node;
    }
    else
    {
        // use one node frome the free list
        LinkNode *node = linkList->free;
        linkList->free = linkList->free->next;
        node->data = (void *)((u8 *)node + sizeof(LinkNode));
        memcpy(node->data, data, linkList->dataSize);
        node->next = 0;
        node->prev = linkList->back;
        linkList->back->next = node;
        linkList->back = linkList->back->next;
        return node;
    }
}

void LinkListRemove(LinkList *linkList, LinkNode *n)
{
    if(n == linkList->front)
    {
        n->next->prev = 0;
        linkList->front = n->next;
    }
    else if(n == linkList->back)
    {
        n->prev->next = 0;
        linkList->back = n->prev;
    }
    else
    {
        n->prev->next = n->next;
        n->next->prev = n->prev;
    }

    n->prev = 0;
    n->next = 0;

    linkList->free = n;

}
*/

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

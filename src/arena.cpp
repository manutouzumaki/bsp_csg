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
#define ArenaPushArray(arena, count, type) ArenaPush(arena, (sizeof(type)*(count)))

#define ArenaTmpBegin(memory, arena) ArenaCreate(memory, arena)
void ArenaTmpEnd(Memory *memory, Arena *arena)
{
    ArenaClear(arena);
    memory->used -= arena->size;
}

//
// ArenaArray implementation
//

#define GetRawData(array) ((u32 *)(array) - 2)
#define GetCapacity(array) (GetRawData(array)[0])
#define GetSize(array) (GetRawData(array)[1])

void *ArenaArrayCreate_(Arena *arena, size_t elementSize, u32 elementCount)
{
    u32 rawSize = sizeof(u32) * 2 + (elementSize * elementCount);
    u32 *base = (u32 *)ArenaPush(arena, rawSize);
    base[0] = elementCount;
    base[1] = 0;
    return (void *)(base + 2);
}

void *ArenaArrayPush_(void *array, size_t elementSize)
{
    ASSERT(array != 0); ASSERT(GetSize(array) + 1 <= GetCapacity(array));
    void *element = (void *)((u8 *)array + (GetSize(array) * elementSize));
    GetSize(array)++;
    return element;
}

#define ArenaArrayCreate(arena, type, count) (type *)ArenaArrayCreate_(arena, sizeof(type), count)
#define ArenaArrayPush(array, element, type)                    \
    do {                                                        \
        type *e = (type *)ArenaArrayPush_(array, sizeof(type)); \
        *e = (element);                                         \
    }while(0); 

u32 ArenaArraySize(void *array)
{
    ASSERT(array != 0);
    return GetSize(array);
}

u32 ArenaArrayCapacity(void *array)
{
    ASSERT(array != 0);
    return GetCapacity(array);
}

void ArenaArrayClear(void *array)
{
    ASSERT(array != 0);
    GetSize(array) = 0;
}

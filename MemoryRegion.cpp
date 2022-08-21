#include "MemoryRegion.hpp"
#include <cstdio>

namespace mreg
{
    MemoryRegionManager::MemoryRegionManager(u64 memblockSizeReserve):
        memblockSizeReserve(memblockSizeReserve)
    {
    }

    MemoryBlock * MemoryRegionManager::allocateBlock(u64 bytes)
    {
        #ifdef MEMORY_REGION_DEBUG
            printf("creating %d bytes block\n", bytes);
        #endif
        u64 size = bytes + sizeof(MemoryBlock);
        MemoryBlock* block = reinterpret_cast<MemoryBlock*>(malloc(size));
        if( block == nullptr )
            throw std::bad_alloc();

        block->end = reinterpret_cast<u8*>(block) + size;
        block->free = reinterpret_cast<u8*>(block) + sizeof(MemoryBlock);
        return block;
    }

    u8 * MemoryRegionManager::allocate(u64 bytes)
    {
        #ifdef MEMORY_REGION_DEBUG
            printf("allocating %d bytes...\n",bytes);
        #endif
        while(tail->free + bytes > tail->end)
        {
            tail->next = released;
            if( released == nullptr )
            {
                tail->next = allocateBlock( memblockSizeReserve + bytes );
            }
            else
            {
                #ifdef MEMORY_REGION_DEBUG
                    printf("  reusing released block\n");
                #endif
                released->free = reinterpret_cast<u8*>(released) + sizeof(MemoryBlock);
                released = released->next;
            }
            tail = tail->next;
            tail->next = nullptr;
        }
        u8 * memory = tail->free;
        tail->free = tail->free + bytes;
        return memory;
    }

    void MemoryRegionManager::release()
    {
        #ifdef MEMORY_REGION_DEBUG
            printf("releasing blocks\n");
        #endif
        tail->next = released;
        released = head.next;
        head.next = nullptr;
        tail = &head;
    }

    MemoryRegionManager::~MemoryRegionManager()
    {
        #ifdef MEMORY_REGION_DEBUG
            printf("deallocating blocks\n");
        #endif
        while(released != nullptr)
        {
            MemoryBlock* next = released->next;
            free(released);
            released = next;
        }
        tail = head.next;
        while(tail != nullptr)
        {
            MemoryBlock* next = tail->next;
            free(tail);
            tail = next;
        }
    }

    MemoryRegionManager::MemoryRegionManager(MemoryRegionManager&& rval)
    {
        head = rval.head;
        released = rval.released;
        tail = rval.tail;

        rval.head = {};
        rval.released = nullptr;
        rval.tail = nullptr;
    }

    void MemoryRegionManager::info() const
    {
        static u64 len = 32;
        MemoryBlock* iter = head.next;
        while(iter)
        {
            u64 size = iter->end - (u8*)(iter) - sizeof(MemoryBlock);
            u64 free =  iter->end - iter->free;
            printf("[");
            float occupied = size - free;
            for(int i = 0; i < len; ++i)
            {
                occupied -= (float)size/len;
                if(occupied >= 0)
                    printf("#");
                else
                    printf(" ");
            }
            printf("] (%d / %d)\n",size - free,size);
            iter  = iter->next;
        }
        iter = released;
        u64 releasedBlocksCounter = 0;
        while(iter)
        {
            releasedBlocksCounter++;
            iter = iter->next;
        }
        printf("Released blocks: %d\n",releasedBlocksCounter);
    }


}
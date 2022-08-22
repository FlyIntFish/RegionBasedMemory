#ifndef MEMORY_REGION
#define MEMORY_REGION
#include <cstdlib>
#include <cstdint>
#include <new>
#include <memory>

namespace mreg
{
    using s64 = int64_t;
    using u64 = uint64_t;
    using u8 = uint8_t;

enum class Unit{
    BYTES,
    KILOBYTES,
    MEGABYTES
};


[[nodiscard]] constexpr s64 kb(s64 amount) { return 1024 * amount;}
[[nodiscard]] constexpr s64 mb(s64 amount) { return 1'048'576 * amount;}
[[nodiscard]] constexpr s64 gb(s64 amount) { return 1'073'741'824 * amount;}

static constexpr u64 DEFAULT_BLOCK_RESERVE = kb(64);


struct MemoryBlock
{
    MemoryBlock * next = nullptr;
    u8 * end = nullptr;
    u8 * free = nullptr;
};

class MemoryRegionManager
{
    MemoryBlock head;
    MemoryBlock * tail = &head;
    MemoryBlock * released = nullptr;
    u64 memblockSizeReserve = DEFAULT_BLOCK_RESERVE;

    MemoryBlock  * allocateBlock(u64 bytes);

    public:
        MemoryRegionManager() = default;
        MemoryRegionManager(u64 memblockSizeReserve);
        ~MemoryRegionManager();
        MemoryRegionManager(const MemoryRegionManager&) = delete;
        MemoryRegionManager(MemoryRegionManager&& rval);

        void release();
        [[nodiscard]] u8 * allocate(u64 bytes);
        void info(Unit unit = Unit::BYTES) const;

        template <typename T, typename... Args>
        T* emplace(Args&&... args);
};

template <typename T>
class RegionAllocator : public std::allocator<T>
{
    MemoryRegionManager * mregAllocator = nullptr;
    public:
        RegionAllocator(MemoryRegionManager * mregAllocator_) : mregAllocator(mregAllocator_){}
        RegionAllocator(RegionAllocator && rval);
        RegionAllocator(const RegionAllocator& cref) = default;
        ~RegionAllocator() = default;

        [[nodiscard]] constexpr T*  allocate(size_t n) const;
        void                        deallocate(T* data, std::size_t size) const {};

        struct rebind
        {
            typedef RegionAllocator other;
        };
};

template <typename T>
[[nodiscard]] constexpr T* RegionAllocator<T>::allocate(size_t n) const
{
    return reinterpret_cast<T*>(mregAllocator->allocate(sizeof(T)*n));
}

template <typename T, typename... Args>
    T* MemoryRegionManager::emplace(Args&&... args)
    {
        T* newObject = reinterpret_cast<T*>(allocate(sizeof(T)));
        ::new (newObject) T(std::forward<Args>(args)...);
        return newObject;
    }

template <typename T>
RegionAllocator<T>::RegionAllocator(RegionAllocator<T> && rval) : mregAllocator(rval.mregAllocator)
{
    rval.mregAllocator = nullptr;
}

} // mreg

#endif // MEMORY_REGION
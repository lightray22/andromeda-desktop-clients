#ifndef LIBA2_SECUREBUFFER_H_
#define LIBA2_SECUREBUFFER_H_

#include <cstring>

namespace Andromeda {

/** 
 * Secure memory allocation functions
 * THREAD SAFE (internal locks)
 */
struct SecureMemory
{
    SecureMemory() = delete; // static only

    /** allocate num number of elements with given size, aligned to size */
    [[nodiscard]] static void* alloc(size_t num, size_t size) noexcept;

    /** free a pointer returned by alloc */
    static void free(void* ptr) noexcept;

    /** allocate num number of T elements */
    template<typename T>
    [[nodiscard]] inline static T* allocT(size_t num) noexcept { 
        return static_cast<T*>(alloc(num, sizeof(T))); }

    /** free a T pointer returned by allocT */
    template<typename T>
    inline static void freeT(T* ptr) noexcept {
        free(static_cast<void*>(ptr)); }
};

/** 
 * Holds a buffer allocated with SecureMemory
 * NOT THREAD SAFE (protect externally)
 */
class SecureBuffer
{
public:
    using T = unsigned char;

    SecureBuffer() = default; // empty

    /** Construct with the given buffer size */
    inline explicit SecureBuffer(size_t size) : 
        mSize(size), mBuf(alloc(mSize)) { }

    inline virtual ~SecureBuffer() noexcept { free(mBuf); }

    inline SecureBuffer(const SecureBuffer& src) noexcept : // copy
        mSize(src.mSize), mBuf(alloc(mSize))
    {
        memcpy(mBuf, src.mBuf, mSize);
    }

    inline SecureBuffer(SecureBuffer&& old) noexcept :
        mSize(old.mSize), mBuf(old.mBuf) // move
    {
        old.mSize = 0;
        old.mBuf = nullptr;
    }

    SecureBuffer& operator=(const SecureBuffer& src) = delete; // copy
    SecureBuffer& operator=(SecureBuffer&& old) = delete; // move

    /** Construct from a nul-terminated c-string (unit test only!) */
    explicit inline SecureBuffer(const char* cstr) :
        mSize(strlen(cstr)), mBuf(alloc(mSize))
    {
        memcpy(mBuf, cstr, mSize);
    }

    /** Construct from bytes from a c-string (unit test only!) */
    explicit inline SecureBuffer(const char* cstr, size_t size) :
        mSize(size), mBuf(alloc(mSize))
    {
        memcpy(mBuf, cstr, mSize);
    }

    /** Compare to another SecureBuffer */
    inline bool operator==(const SecureBuffer& rhs) const
    {
        return mSize == rhs.mSize &&
            !memcmp(mBuf, rhs.mBuf, mSize);
    }

    /** Compare to another c-string (unit test only!) */
    inline bool operator==(const char* cstr) const
    {
        return mSize == strlen(cstr) &&
            !memcmp(mBuf, cstr, mSize);
    }

    /** Returns a pointer to the secure buffer */
    [[nodiscard]] inline T* data() noexcept { return mBuf; }
    /** Returns a pointer to the secure buffer */
    [[nodiscard]] inline const T* data() const noexcept { return mBuf; }
    /** Returns the size of the secure buffer */
    [[nodiscard]] inline size_t size() const noexcept { return mSize; }

    /** Reallocates the buffer to the given size, copies data */
    inline void resize(size_t newSize) noexcept
    {
        T* newBuf = alloc(newSize);
        memcpy(newBuf, mBuf, std::min(newSize,mSize));
        free(mBuf);
        
        mSize = newSize;
        mBuf = newBuf;
    }

    /** Returns a new SecureBuffer whose content is size# bytes from offset */
    [[nodiscard]] inline SecureBuffer substr(size_t offset, size_t size) const noexcept
    {
        SecureBuffer ret(size);
        memcpy(ret.data(), mBuf+offset, size);
        return ret;
    }

private:

    [[nodiscard]] inline static T* alloc(size_t size) noexcept { 
        return SecureMemory::allocT<T>(size); }

    inline static void free(T* ptr) noexcept { 
        SecureMemory::freeT<T>(ptr); }

    size_t mSize { 0 };
    T* mBuf { nullptr };
};

} // namespace Andromeda

#endif // LIBA2_SECUREBUFFER_H_

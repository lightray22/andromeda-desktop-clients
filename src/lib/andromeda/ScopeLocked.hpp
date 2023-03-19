
#ifndef LIBA2_SCOPELOCKEDT_H_
#define LIBA2_SCOPELOCKEDT_H_

#include <shared_mutex>
#include <type_traits>

namespace Andromeda {

/** 
 * Lock that protects classes that may be deleted or go out of scope by acquiring a 
 * shared mutex, which the class should acquire exclusively before going out of scope.  
 * Operator bool returns true/false to indicate if the object was locked or not and must always be checked!
 */
template<class Object>
class ScopeLocked
{
public:
    /** Construct an empty object, not locked, don't reference */
    inline ScopeLocked(){ } // empty

    /** Construct a lock from an object and a mutex to try to lock */
    inline ScopeLocked(Object& obj, std::shared_mutex& mutex) :
        mObject(&obj), mLock(mutex, std::try_to_lock) { }

    /** Return a reference to the locked object */
    inline Object& operator*(){ return *mObject; }
    
    /** Return a const reference to the locked object */
    inline const Object& operator*()const{ return *mObject; }
    
    /** Return a pointer to the locked object */
    inline Object* operator->(){ return mObject; }

    /** Return a const pointer to the locked object */
    inline const Object* operator->()const{ return mObject; }

    /** Returns true if the lock acquire was successful */
    inline operator bool() const { return static_cast<bool>(mLock); }

    /** Unlocks the currently held lock */
    inline void unlock(){ mLock.unlock(); }

    // all specializations are friends
    template<class> friend class ScopeLocked;

    /** 
     * Move a lock to one with mObject dynamic_cast()'d as a child class
     * The lock moved from will still have a valid object ref but no lock
     */
    template<class Base>
    inline static ScopeLocked<Object> FromBase(ScopeLocked<Base>&& slock)
    {
        static_assert(std::is_base_of<Base, Object>());
        return ScopeLocked(dynamic_cast<Object&>(*slock.mObject), std::move(slock.mLock));
    }

    /** 
     * Move a lock to one with mObject static_cast'd as a base class 
     * The lock moved from will still have a valid object ref but no lock
     */
    template<class Child> // child class
    inline static ScopeLocked<Object> FromChild(ScopeLocked<Child>&& slock)
    {
        return ScopeLocked(static_cast<Object&>(*slock.mObject), std::move(slock.mLock));
    }

protected:
    inline ScopeLocked(Object& obj, std::shared_lock<std::shared_mutex>&& lock) :
        mObject(&obj), mLock(std::move(lock)) { }

private:
    Object* mObject { nullptr }; 
    std::shared_lock<std::shared_mutex> mLock;
};

} // namespace Andromeda

#endif // LIBA2_SCOPELOCKEDT_H_

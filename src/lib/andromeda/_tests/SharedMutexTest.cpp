
#include <chrono>
#include <list>
#include <mutex>
#include <thread>

#include "catch2/catch_test_macros.hpp"

#include "SharedMutex.hpp"

// Yes obviously these tests are full of race conditions and timing assumptions
// ... don't run these regularly unless doing development on this class

namespace Andromeda {
namespace { // anonymous

using Results = std::list<std::string>;

enum class LockType : uint8_t { WRITE, READ, READP };

void wait(const size_t mstime) 
{ 
    std::this_thread::sleep_for(
        std::chrono::milliseconds(mstime)); 
}

void RunLock(SharedMutex& mut, Results& res, 
    std::mutex& resMutex, const std::string& name, LockType type)
{
    switch (type)
    {
        case LockType::WRITE: mut.lock(); break;
        case LockType::READ: mut.lock_shared(); break;
        case LockType::READP: mut.lock_shared(true); break;
    }

    { // lock scope
        const std::lock_guard<std::mutex> resLock(resMutex);
        res.push_back(name+"_lock"); 
    }
}

void RunUnlock(SharedMutex& mut, Results& res, 
    std::mutex& resMutex, const std::string& name, LockType type)
{
    { // lock scope
        const std::lock_guard<std::mutex> resLock(resMutex);
        res.push_back(name+"_unlock"); 
    }

    switch (type)
    {
        case LockType::WRITE: mut.unlock(); break;
        case LockType::READ: mut.unlock_shared(); break;
        case LockType::READP: mut.unlock_shared(); break;
    }
}

void RunTimed(SharedMutex& mut, Results& res, std::mutex& resMutex, 
    const char* const name, const size_t mstime, LockType type)
    // name is not a string ref because it would go out of scope (thread)
{
    RunLock(mut, res, resMutex, name, type);
    wait(mstime);
    RunUnlock(mut, res, resMutex, name, type);
}

#define RunThread(tn, name, mstime, type) \
    std::thread tn(RunTimed, std::ref(mut), std::ref(res), std::ref(resMutex), name, mstime, type);

/*****************************************************/
TEST_CASE("TestRWP", "[SharedMutex]") 
{
    SharedMutex mut; Results res; std::mutex resMutex;

    RunLock(mut, res, resMutex, "1", LockType::READ);
    RunThread(t2, "2", 100, LockType::WRITE); wait(30);
    RunThread(t3, "3", 100, LockType::READP); wait(30);

    RunUnlock(mut, res, resMutex, "1", LockType::READ);
    t2.join(); t3.join();

    REQUIRE(res == Results{"1_lock", "3_lock", "1_unlock", "3_unlock", "2_lock", "2_unlock"});
}

/*****************************************************/
TEST_CASE("TestRPW", "[SharedMutex]") 
{
    SharedMutex mut; Results res; std::mutex resMutex;

    RunLock(mut, res, resMutex, "1", LockType::READ);
    RunThread(t2, "2", 200, LockType::READP); wait(30);
    RunThread(t3, "3", 100, LockType::WRITE); wait(30);

    RunUnlock(mut, res, resMutex, "1", LockType::READ);
    t2.join(); t3.join();

    REQUIRE(res == Results{"1_lock", "2_lock", "1_unlock", "2_unlock", "3_lock", "3_unlock"});
}

/*****************************************************/
TEST_CASE("TestWPR", "[SharedMutex]") 
{
    SharedMutex mut; Results res; std::mutex resMutex;

    RunLock(mut, res, resMutex, "1", LockType::WRITE);
    RunThread(t2, "2", 100, LockType::READP); wait(30);
    RunThread(t3, "3", 150, LockType::READ); wait(30);

    RunUnlock(mut, res, resMutex, "1", LockType::WRITE);
    t2.join(); t3.join();

    REQUIRE(res == Results{"1_lock", "1_unlock", "2_lock", "3_lock", "2_unlock", "3_unlock"});
}

/*****************************************************/
TEST_CASE("TestPWR", "[SharedMutex]") 
{
    SharedMutex mut; Results res; std::mutex resMutex;

    RunLock(mut, res, resMutex, "1", LockType::READP);
    RunThread(t2, "2", 100, LockType::WRITE); wait(30);
    RunThread(t3, "3", 100, LockType::READ); wait(30);

    RunUnlock(mut, res, resMutex, "1", LockType::READP);
    t2.join(); t3.join();

    REQUIRE(res == Results{"1_lock", "1_unlock", "2_lock", "2_unlock", "3_lock", "3_unlock"});
}

/*****************************************************/
TEST_CASE("TestPRW", "[SharedMutex]") 
{
    SharedMutex mut; Results res; std::mutex resMutex;

    RunLock(mut, res, resMutex, "1", LockType::READP);
    RunThread(t2, "2", 100, LockType::READ); wait(30);
    RunThread(t3, "3", 100, LockType::WRITE); wait(30);

    RunUnlock(mut, res, resMutex, "1", LockType::READP);
    t2.join(); t3.join();

    REQUIRE(res == Results{"1_lock", "2_lock", "1_unlock", "2_unlock", "3_lock", "3_unlock"});
}

/*****************************************************/
TEST_CASE("TestRPWR", "[SharedMutex]")
{
    SharedMutex mut; Results res; std::mutex resMutex;

    RunLock(mut, res, resMutex, "1", LockType::READ);
    RunThread(t2, "2", 100, LockType::READP); wait(30);
    RunThread(t3, "3", 100, LockType::WRITE); wait(30);

    RunUnlock(mut, res, resMutex, "1", LockType::READ);
    RunThread(t4, "4", 100, LockType::READ); wait(30);

    t2.join(); // t2 done, run t3
    t3.join(); // t3 done, run t4
    t4.join();

    REQUIRE(res == Results{"1_lock", "2_lock", "1_unlock", "2_unlock", "3_lock", "3_unlock", "4_lock", "4_unlock"});
}

/*****************************************************/
TEST_CASE("TestTryLock", "[SharedMutex]")
{
    SharedMutex mut;

    mut.lock();
    REQUIRE(mut.try_lock() == false);
    mut.unlock();

    mut.lock_shared();
    REQUIRE(mut.try_lock() == false);
    mut.unlock();

    REQUIRE(mut.try_lock() == true);
    REQUIRE(mut.try_lock() == false);
    mut.unlock();
}

} // namespace
} // namespace Andromeda

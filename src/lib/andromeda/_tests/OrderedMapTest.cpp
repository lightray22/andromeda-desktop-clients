
#include <iterator>
#include <string>

#include "catch2/catch_test_macros.hpp"

#include "OrderedMap.hpp"

namespace Andromeda {
namespace { // anonymous

using TestM = OrderedMap<int, std::string>;
using TestMV = TestM::value_type;
using TestQ = HashedQueue<int>;

/*****************************************************/
TEST_CASE("TestBasic", "[OrderedMap]")
{
    TestM testM; TestQ testQ;
    testM.enqueue_front(5, "myval"); testQ.enqueue_front(5);
    testM.enqueue_front(7, "myval2"); testQ.enqueue_front(7);
    testM.enqueue_back(9, "myval3"); testQ.enqueue_back(9);

    const TestMV e1{7, "myval2"}; const int v1 = 7;
    const TestMV e2(5, "myval"); const int v2 = 5;
    const TestMV e3(9, "myval3"); const int v3 = 9;

    REQUIRE(testM == TestM{e1, e2, e3}); REQUIRE(testQ == TestQ{v1, v2, v3});
    REQUIRE(testM.size() == 3); REQUIRE(testQ.size() == 3);
    REQUIRE(!testM.empty()); REQUIRE(!testQ.empty());
    REQUIRE(testM.front() == e1); REQUIRE(testQ.front() == v1);
    REQUIRE(testM.back() == e3); REQUIRE(testQ.back() == v3);

    REQUIRE(*testM.begin() == e1); REQUIRE(*testQ.begin() == v1);
    REQUIRE(*testM.cbegin() == e1); REQUIRE(*testQ.cbegin() == v1);
    REQUIRE(*testM.rbegin() == e3); REQUIRE(*testQ.rbegin() == v3);
    REQUIRE(*testM.crbegin() == e3); REQUIRE(*testQ.crbegin() == v3);

    REQUIRE(*std::prev(testM.end()) == e3); REQUIRE(*std::prev(testQ.end()) == v3);
    REQUIRE(*std::prev(testM.cend()) == e3); REQUIRE(*std::prev(testQ.cend()) == v3);
    REQUIRE(*std::prev(testM.rend()) == e1); REQUIRE(*std::prev(testQ.rend()) == v1);
    REQUIRE(*std::prev(testM.crend()) == e1); REQUIRE(*std::prev(testQ.crend()) == v1);
    
    testM.clear(); testQ.clear();
    REQUIRE(testM == TestM{}); REQUIRE(testQ == TestQ{}); // NOLINT(readability-container-size-empty)
    REQUIRE(testM.size() == 0); REQUIRE(testQ.size() == 0); // NOLINT(readability-container-size-empty)
    REQUIRE(testM.empty()); REQUIRE(testQ.empty());
    REQUIRE(testM.begin() == testM.end()); REQUIRE(testQ.begin() == testQ.end());
}

/*****************************************************/
TEST_CASE("TestFindErase", "[OrderedMap]")
{
    TestM testM; TestQ testQ;
    testM.enqueue_front(5, "myval"); testQ.enqueue_front(5);
    const TestM::iterator itM { testM.find(5) }; const TestQ::iterator itQ { testQ.find(5) };

    testM.enqueue_front(7, "myval2"); testQ.enqueue_front(7);
    testM.enqueue_front(9, "myval3"); testQ.enqueue_front(9);

    REQUIRE(testM.exists(5)); REQUIRE(testQ.exists(5));
    REQUIRE(!testM.exists(15)); REQUIRE(!testQ.exists(15));

    REQUIRE(*testM.find(5) == TestMV{5,"myval"}); REQUIRE(*testQ.find(5) == 5);
    REQUIRE(*testM.find(7) == TestMV{7,"myval2"}); REQUIRE(*testQ.find(7) == 7);
    REQUIRE(*testM.find(9) == TestMV{9,"myval3"}); REQUIRE(*testQ.find(9) == 9);
    REQUIRE(testM.find(11) == testM.end()); REQUIRE(testQ.find(11) == testQ.end());

    testM.erase(testM.lookup(7)); testQ.erase(testQ.lookup(7));
    REQUIRE(testM == TestM{{9,"myval3"},{5,"myval"}}); REQUIRE(testQ == TestQ{9,5});

    REQUIRE(testM.erase(9)); REQUIRE(testQ.erase(9));
    REQUIRE(testM == TestM{{5,"myval"}}); REQUIRE(testQ == TestQ{5});

    // iterators should stay valid after erases
    REQUIRE(*itM == TestMV{5,"myval"}); REQUIRE(*itQ == 5);

    REQUIRE(testM.erase(testM.lookup(5)) == testM.lend()); REQUIRE(testQ.erase(testQ.lookup(5)) == testQ.lend());
    REQUIRE(testM.size() == 0); REQUIRE(testQ.size() == 0); // NOLINT(readability-container-size-empty)
    REQUIRE(testM == TestM{}); REQUIRE(testQ == TestQ{}); // NOLINT(readability-container-size-empty)
}

/*****************************************************/
TEST_CASE("TestPop", "[OrderedMap]")
{
    TestM testM; TestQ testQ;
    testM.enqueue_front(5, "myval"); testQ.enqueue_front(5);
    testM.enqueue_front(7, "myval2"); testQ.enqueue_front(7);
    testM.enqueue_front(9, "myval3"); testQ.enqueue_front(9);
    testM.enqueue_front(11, "myval4"); testQ.enqueue_front(11);

    std::string s7; int i7 = 0;
    REQUIRE(testM.pop(7, s7)); REQUIRE(testQ.pop(7, i7));
    REQUIRE(!testM.pop(15, s7)); REQUIRE(!testQ.pop(15, i7));
    REQUIRE(s7 == "myval2"); REQUIRE(i7 == 7);
    REQUIRE(testM == TestM{{11,"myval4"},{9,"myval3"},{5,"myval"}}); REQUIRE(testQ == TestQ{11,9,5});

    TestMV s11 { testM.pop_front() }; const int i11 { testQ.pop_front() };
    REQUIRE(s11 == TestMV{11, "myval4"}); REQUIRE(i11 == 11);
    REQUIRE(testM == TestM{{9,"myval3"},{5,"myval"}}); REQUIRE(testQ == TestQ{9,5});

    TestMV s5 { testM.pop_back() }; const int i5 { testQ.pop_back() };
    REQUIRE(s5 == TestMV{5, "myval"}); REQUIRE(i5 == 5);
    REQUIRE(testM == TestM{{9,"myval3"}}); REQUIRE(testQ == TestQ{9});
}

} // namespace
} // namespace Andromeda

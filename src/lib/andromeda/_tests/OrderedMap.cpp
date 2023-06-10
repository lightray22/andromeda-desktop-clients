
#include <iterator>
#include <string>

#include "catch2/catch_test_macros.hpp"

#include "OrderedMap.hpp"

namespace Andromeda {

typedef OrderedMap<int, std::string> TestM;
typedef TestM::value_type TestMV;
typedef HashedQueue<int> TestQ;

/*****************************************************/
TEST_CASE("TestBasic", "[OrderedMap]")
{
    TestM testM; TestQ testQ;
    testM.enqueue_front(5, "myval"); testQ.enqueue_front(5);
    testM.enqueue_front(7, "myval2"); testQ.enqueue_front(7);

    TestMV e1{7, "myval2"}; int v1 = 7;
    TestMV e2(5, "myval"); int v2 = 5;

    REQUIRE(testM == TestM{e1, e2}); REQUIRE(testQ == TestQ{v1, v2});
    REQUIRE(testM.size() == 2); REQUIRE(testQ.size() == 2);
    REQUIRE(testM.empty() == false); REQUIRE(testQ.empty() == false);
    REQUIRE(testM.front() == e1); REQUIRE(testQ.front() == v1);
    REQUIRE(testM.back() == e2); REQUIRE(testQ.back() == v2);

    REQUIRE(*testM.begin() == e1); REQUIRE(*testQ.begin() == v1);
    REQUIRE(*testM.cbegin() == e1); REQUIRE(*testQ.cbegin() == v1);
    REQUIRE(*testM.rbegin() == e2); REQUIRE(*testQ.rbegin() == v2);
    REQUIRE(*testM.crbegin() == e2); REQUIRE(*testQ.crbegin() == v2);

    REQUIRE(*std::prev(testM.end()) == e2); REQUIRE(*std::prev(testQ.end()) == v2);
    REQUIRE(*std::prev(testM.cend()) == e2); REQUIRE(*std::prev(testQ.cend()) == v2);
    REQUIRE(*std::prev(testM.rend()) == e1); REQUIRE(*std::prev(testQ.rend()) == v1);
    REQUIRE(*std::prev(testM.crend()) == e1); REQUIRE(*std::prev(testQ.crend()) == v1);
    
    testM.clear(); testQ.clear();
    REQUIRE(testM == TestM{}); REQUIRE(testQ == TestQ{});
    REQUIRE(testM.size() == 0); REQUIRE(testQ.size() == 0);
    REQUIRE(testM.empty() == true); REQUIRE(testQ.empty() == true);
    REQUIRE(testM.begin() == testM.end()); REQUIRE(testQ.begin() == testQ.end());
}

/*****************************************************/
TEST_CASE("TestFindErase", "[OrderedMap]")
{
    TestM testM; TestQ testQ;
    testM.enqueue_front(5, "myval"); testQ.enqueue_front(5);
    TestM::iterator itM { testM.find(5) }; TestQ::iterator itQ { testQ.find(5) };

    testM.enqueue_front(7, "myval2"); testQ.enqueue_front(7);
    testM.enqueue_front(9, "myval3"); testQ.enqueue_front(9);

    REQUIRE(*testM.find(5) == TestMV{5,"myval"}); REQUIRE(*testQ.find(5) == 5);
    REQUIRE(*testM.find(7) == TestMV{7,"myval2"}); REQUIRE(*testQ.find(7) == 7);
    REQUIRE(*testM.find(9) == TestMV{9,"myval3"}); REQUIRE(*testQ.find(9) == 9);
    REQUIRE(testM.find(11) == testM.end()); REQUIRE(testQ.find(11) == testQ.end());

    testM.erase(testM.find(7)); testQ.erase(testQ.find(7));
    REQUIRE(testM == TestM{{9,"myval3"},{5,"myval"}}); REQUIRE(testQ == TestQ{9,5});

    REQUIRE(testM.erase(9) == true); REQUIRE(testQ.erase(9) == true);
    REQUIRE(testM == TestM{{5,"myval"}}); REQUIRE(testQ == TestQ{5});

    // iterators should stay valid after erases
    REQUIRE(*itM == TestMV{5,"myval"}); REQUIRE(*itQ == 5);

    REQUIRE(testM.erase(testM.begin()) == testM.end()); REQUIRE(testQ.erase(testQ.begin()) == testQ.end());
    REQUIRE(testM.size() == 0); REQUIRE(testQ.size() == 0);
    REQUIRE(testM == TestM{}); REQUIRE(testQ == TestQ{}); // test empty ==
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
    REQUIRE(testM.pop(7, s7) == true); REQUIRE(testQ.pop(7, i7) == true);
    REQUIRE(testM.pop(15, s7) == false); REQUIRE(testQ.pop(15, i7) == false);
    REQUIRE(s7 == "myval2"); REQUIRE(i7 == 7);
    REQUIRE(testM == TestM{{11,"myval4"},{9,"myval3"},{5,"myval"}}); REQUIRE(testQ == TestQ{11,9,5});

    TestMV s11 { testM.pop_front() }; int i11 { testQ.pop_front() };
    REQUIRE(s11 == TestMV{11, "myval4"}); REQUIRE(i11 == 11);
    REQUIRE(testM == TestM{{9,"myval3"},{5,"myval"}}); REQUIRE(testQ == TestQ{9,5});

    TestMV s5 { testM.pop_back() }; int i5 { testQ.pop_back() };
    REQUIRE(s5 == TestMV{5, "myval"}); REQUIRE(i5 == 5);
    REQUIRE(testM == TestM{{9,"myval3"}}); REQUIRE(testQ == TestQ{9});
}

} // namespace Andromeda

#ifndef LIBA2_ORDEREDMAP_H_
#define LIBA2_ORDEREDMAP_H_

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <list>
#include <unordered_map>
#include <utility>

namespace Andromeda {

/**
 * Combines a std::list and a std::unordered_map to provide a) a list with quick lookup or b) a map that keeps insertion order
 * This abstract base allows specifying the type of value stored in the map and how to get the key/value from it
 * The semantics of this class are meant to be very similar to the std::map and std::list
 * @tparam Key the class that is the Key used for hashmap lookup
 * @tparam Value the class that is the Value being mapped to externally
 * @tparam Entry the class that is the actual type stored in the list
 * @tparam Impl the implementor class that specifies how to get the key/value from the Entry
 */
template<typename Key, typename Value, typename Entry, typename Impl>
class OrderedMapAnyEntry
{
public:
    using value_type = Entry;
    using ValueQueue = std::list<value_type>;
    using iterator = typename ValueQueue::iterator;
    using const_iterator = typename ValueQueue::const_iterator;
    using reverse_iterator = typename ValueQueue::reverse_iterator;
    using const_reverse_iterator = typename ValueQueue::const_reverse_iterator;

    using ValueLookup = std::unordered_map<Key, iterator>;
    using lookup_iterator = typename ValueLookup::iterator;
    using const_lookup_iterator = typename ValueLookup::const_iterator;

    /** Returns the map key from the given value_type */
    [[nodiscard]] inline Key get_key(const value_type& e) noexcept {
        return static_cast<Impl*>(this)->get_key(e);
    }
    /** Returns the map value from the given value_type */
    [[nodiscard]] inline Value get_value(const value_type& e) noexcept {
        return static_cast<Impl*>(this)->get_value(e);
    }

    /** Returns an iterator pointing to the first element in the list */
    [[nodiscard]] inline iterator begin() noexcept { return mQueue.begin(); }
    /** Returns an iterator pointing to the past-the-end element in the list */
    [[nodiscard]] inline iterator end() noexcept { return mQueue.end(); }
    /** Returns a const iterator pointing to the first element in the list */
    [[nodiscard]] inline const_iterator cbegin() const noexcept { return mQueue.cbegin(); }
    /** Returns a const iterator pointing to the past-the-end element in the list */
    [[nodiscard]] inline const_iterator cend() const noexcept { return mQueue.cend(); }
    /** Returns a reverse iterator pointing to the last element in the list */
    [[nodiscard]] inline reverse_iterator rbegin() noexcept { return mQueue.rbegin(); }
    /** Returns a reverse iterator pointing to the before-the-start element in the list */
    [[nodiscard]] inline reverse_iterator rend() noexcept { return mQueue.rend(); }
    /** Returns a const reverse iterator pointing to the last element in the list */
    [[nodiscard]] inline const_reverse_iterator crbegin() const noexcept { return mQueue.crbegin(); }
    /** Returns a const reverse iterator pointing to the before-the-start element in the list */
    [[nodiscard]] inline const_reverse_iterator crend() const noexcept { return mQueue.crend(); }

    /** Returns an end iterator for the lookup table (use with lookup) */
    [[nodiscard]] inline lookup_iterator lend() noexcept { return mLookup.end(); }
    /** Returns a const end iterator for the lookup table (use with lookup) */
    [[nodiscard]] inline const_lookup_iterator lcend() noexcept { return mLookup.cend(); };

    /** Returns a reference to the first element in the list (MUST NOT BE EMPTY) (O(1)) */
    [[nodiscard]] inline value_type& front() noexcept { return mQueue.front(); }
    /** Returns a reference to the last element in the list (MUST NOT BE EMPTY) (O(1)) */
    [[nodiscard]] inline value_type& back() noexcept { return mQueue.back(); }
    /** Returns the number of elements in the list (O(1)) */
    [[nodiscard]] inline size_t size() const noexcept { return mQueue.size(); }
    /** Returns true iff the list is empty (O(1)) */
    [[nodiscard]] inline bool empty() const noexcept { return mQueue.empty(); }

    /** Empties the list and hash map (O(1)) */
    inline void clear() noexcept
    {
        mQueue.clear();
        mLookup.clear();
    }

    explicit OrderedMapAnyEntry() = default;

    OrderedMapAnyEntry(const std::initializer_list<value_type>& list) noexcept
    {
        for (const value_type& entry : list)
        {
            mQueue.emplace_back(entry); // order matters
            mLookup[get_key(entry)] = std::prev(mQueue.end());
        }
    }

    bool operator==(const OrderedMapAnyEntry<Key, Value, Entry, Impl>& rhs) const noexcept
    {
        if (size() != rhs.size()) return false;
        if (!size() && !rhs.size()) return true;
        const_iterator itL { cbegin() }; 
        const_iterator itR { rhs.cbegin() };

        for (; itL != cend(); ++itL, ++itR)
            if (*itL != *itR) return false;
        return true;
    }

    /** Returns true if the key exists in the map */
    [[nodiscard]] bool exists(const Key& key) const noexcept
    {
        return mLookup.find(key) != mLookup.end();
    }

    /**
     * Returns an iterator to the element with the given key, or end() if not found
     * Complexity: O(1) average, O(N) worst
     */
    [[nodiscard]] iterator find(const Key& key) noexcept
    {
        const lookup_iterator itLookup { mLookup.find(key) }; // O(1)-O(n)
        if (itLookup == mLookup.end()) return mQueue.end();
        else return itLookup->second;
    }

    /**
     * Returns an iterator to the element with the given key, or end() if not found
     * Complexity: O(1) average, O(N) worst
     */
    [[nodiscard]] const_iterator find(const Key& key) const noexcept
    {
        const lookup_iterator itLookup { mLookup.find(key) }; // O(1)-O(n)
        if (itLookup == mLookup.end()) return mQueue.end();
        else return itLookup->second;
    }

    /**
     * Returns an iterator to the element with the given key, or end() if not found
     * Complexity: O(1) average, O(N) worst
     */
    [[nodiscard]] lookup_iterator lookup(const Key& key) noexcept
    {
        return mLookup.find(key); // O(1)-O(n)
    }

    /**
     * Returns an iterator to the element with the given key, or end() if not found
     * Complexity: O(1) average, O(N) worst
     */
    [[nodiscard]] const_lookup_iterator lookup(const Key& key) const noexcept
    {
        return mLookup.find(key); // O(1)-O(n)
    }

    /**
     * Erases the element pointed to by the given iterator
     * @return iterator pointing to the next element in the container
     * Complexity: O(1) average, O(N) worst
     */
    lookup_iterator erase(const lookup_iterator& it) noexcept
    {
        mQueue.erase(it->second); // O(1)-O(n)
        return mLookup.erase(it); // O(1)
    }

    /**
     * Looks up and erases the element with the given key
     * @return true iff an element was found and erased
     * Complexity: O(1) average, O(N) worst
     */
    bool erase(const Key& key) noexcept
    {
        const lookup_iterator itLookup { mLookup.find(key) }; // O(1)-O(n)
        if (itLookup == mLookup.end()) return false;

        mQueue.erase(itLookup->second); // O(1)
        mLookup.erase(itLookup); // O(1)
        return true;
    }

    /**
     * Looks up, erases and returns the value with the given key
     * @param[out] val reference to the Value to set
     * @return true iff an element was found and returned
     * Complexity: O(1) average, O(N) worst
     */
    bool pop(const Key& key, Value& val) noexcept
    {
        const lookup_iterator itLookup { mLookup.find(key) }; // O(1)-O(n)
        if (itLookup == mLookup.end()) return false;

        val = std::move(get_value(*itLookup->second));
        mQueue.erase(itLookup->second); // O(1)
        mLookup.erase(itLookup); // O(1)
        return true;
    }

    /**
     * Erases and returns the first element in the list (MUST NOT BE EMPTY)
     * Complexity: O(1) average, O(N) worst
     */
    [[nodiscard]] value_type pop_front() noexcept
    {
        const iterator itQueue { mQueue.begin() };
        mLookup.erase(get_key(*itQueue)); // O(1)-O(n)
        value_type v { std::move(*itQueue) }; // NOLINT(misc-const-correctness)
        mQueue.erase(itQueue); // O(1)
        return v; // non-const for move
    }
    
    /**
     * Erases and returns the last element in the list (MUST NOT BE EMPTY)
     * Complexity: O(1) average, O(N) worst
     */
    [[nodiscard]] value_type pop_back() noexcept
    {
        const iterator itQueue { std::prev(mQueue.end()) };
        mLookup.erase(get_key(*itQueue)); // O(1)-O(n)
        value_type v { std::move(*itQueue) }; // NOLINT(misc-const-correctness)
        mQueue.erase(itQueue); // O(1)
        return v; // non-const for move
    }

    /**
     * Emplaces a new element on the front of the list (KEY MUST NOT EXIST)
     * Complexity: O(1) average, O(N) worst
     */
    template<class... Ts>
    void enqueue_front(Ts&&... args) noexcept
    {
    #if DEBUG // key must not exist
        value_type v(std::forward<Ts>(args)...); // NOLINT(misc-const-correctness)
        assert(!erase(get_key(v)));
        mQueue.emplace_front(std::move(v)); // O(1) // non-const for move
    #else // !DEBUG
        mQueue.emplace_front(std::forward<Ts>(args)...); // O(1)
    #endif // DEBUG
        mLookup[get_key(mQueue.front())] = mQueue.begin(); // O(1)-O(n)
    }

    /**
     * Emplaces a new element on the end of the list (KEY MUST NOT EXIST)
     * Complexity: O(1) average, O(N) worst
     */
    template<class... Ts>
    void enqueue_back(Ts&&... args) noexcept
    {
    #if DEBUG // key must not exist
        value_type v(std::forward<Ts>(args)...); // NOLINT(misc-const-correctness)
        assert(!erase(get_key(v)));
        mQueue.emplace_back(std::move(v)); // O(1) // non-const for move
    #else // !DEBUG
        mQueue.emplace_back(std::forward<Ts>(args)...); // O(1)
    #endif // DEBUG
        mLookup[get_key(mQueue.back())] = std::prev(mQueue.end()); // O(1)-O(n)
    }

private:
    ValueQueue mQueue;
    ValueLookup mLookup;
};

/** A std::unordered_map (fast lookup) that also acts as an ordered queue (keeps insertion order) */
template<typename Key, typename Value>
class OrderedMap : public OrderedMapAnyEntry<Key, Value, std::pair<const Key, Value>, OrderedMap<Key, Value>>
{
public:
    using value_type = std::pair<const Key, Value>;
    [[nodiscard]] inline Key get_key(const value_type& e) noexcept { return e.first; }
    [[nodiscard]] inline Value get_value(const value_type& e) noexcept { return e.second; }
    using OrderedMapAnyEntry<Key, Value, std::pair<const Key, Value>, OrderedMap<Key, Value>>::OrderedMapAnyEntry;
};

/** 
 * A std::list (ordered) that provides fast lookup using a hash map (must be unique values)
 * This is an OrderedMap but with Values as Keys, and the value_type is just the Value not a pair
 */
template<typename Value>
class HashedQueue : public OrderedMapAnyEntry<Value, Value, Value, HashedQueue<Value>>
{
public:
    using value_type = Value;
    [[nodiscard]] inline Value get_key(const value_type& e) noexcept { return e; }
    [[nodiscard]] inline Value get_value(const value_type& e) noexcept { return e; }
    using OrderedMapAnyEntry<Value, Value, Value, HashedQueue<Value>>::OrderedMapAnyEntry;
};

} // namespace Andromeda

#endif // LIBA2_ORDEREDMAP_H_

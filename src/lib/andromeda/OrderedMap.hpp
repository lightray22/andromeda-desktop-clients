
#ifndef LIBA2_ORDEREDMAP_H_
#define LIBA2_ORDEREDMAP_H_

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

    /** Returns the map key from the given value_type */
    inline Key get_key(const value_type& e) noexcept{
        return static_cast<Impl*>(this)->get_key(e);
    }
    /** Returns the map value from the given value_type */
    inline Value get_value(const value_type& e) noexcept{
        return static_cast<Impl*>(this)->get_value(e);
    }

    /** Returns an iterator pointing to the first element in the list */
    inline iterator begin() noexcept { return mQueue.begin(); }
    /** Returns an iterator pointing to the past-the-end element in the list */
    inline iterator end() noexcept { return mQueue.end(); }
    /** Returns a const iterator pointing to the first element in the list */
    inline const_iterator cbegin() const noexcept { return mQueue.cbegin(); }
    /** Returns a const iterator pointing to the past-the-end element in the list */
    inline const_iterator cend() const noexcept { return mQueue.cend(); }
    /** Returns a reverse iterator pointing to the last element in the list */
    inline reverse_iterator rbegin() noexcept { return mQueue.rbegin(); }
    /** Returns a reverse iterator pointing to the before-the-start element in the list */
    inline reverse_iterator rend() noexcept { return mQueue.rend(); }
    /** Returns a const reverse iterator pointing to the last element in the list */
    inline const_reverse_iterator crbegin() const noexcept { return mQueue.crbegin(); }
    /** Returns a const reverse iterator pointing to the before-the-start element in the list */
    inline const_reverse_iterator crend() const noexcept { return mQueue.crend(); }

    /** Returns a reference to the first element in the list (MUST NOT BE EMPTY) (O(1)) */
    inline value_type& front() noexcept { return mQueue.front(); }
    /** Returns a reference to the last element in the list (MUST NOT BE EMPTY) (O(1)) */
    inline value_type& back() noexcept { return mQueue.back(); }
    /** Returns the number of elements in the list (O(1)) */
    inline size_t size() const noexcept { return mQueue.size(); }
    /** Returns true iff the list is empty (O(1)) */
    inline bool empty() const noexcept { return mQueue.empty(); }

    /** Empties the list and hash map (O(1)) */
    inline void clear() noexcept
    {
        mQueue.clear();
        mLookup.clear();
    }

    explicit OrderedMapAnyEntry() = default;

    explicit OrderedMapAnyEntry(const std::initializer_list<value_type>& list) noexcept
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

    /**
     * Returns an iterator to the element with the given key, or end() if not found
     * Complexity: O(1) average, O(N) worst
     */
    iterator find(const Key& key) noexcept
    {
        lookup_iterator itLookup { mLookup.find(key) }; // O(1)-O(n)
        if (itLookup == mLookup.end()) return mQueue.end();
        else return itLookup->second;
    }

    /**
     * Erases the element pointed to by the given iterator
     * @return iterator pointing to the next element in the container
     * Complexity: O(1) average, O(N) worst
     */
    iterator erase(const iterator& it) noexcept
    {
        mLookup.erase(get_key(*it)); // O(1)-O(n)
        return mQueue.erase(it); // O(1)
    }

    /**
     * Looks up and erases the element with the given key
     * @return true iff an element was found and erased
     * Complexity: O(1) average, O(N) worst
     */
    bool erase(const Key& key) noexcept
    {
        lookup_iterator itLookup { mLookup.find(key) }; // O(1)-O(n)
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
        lookup_iterator itLookup { mLookup.find(key) }; // O(1)-O(n)
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
    value_type pop_front() noexcept
    {
        iterator itQueue { mQueue.begin() };
        mLookup.erase(get_key(*itQueue)); // O(1)-O(n)
        value_type v { std::move(*itQueue) };
        mQueue.erase(itQueue); // O(1)
        return v;
    }
    
    /**
     * Erases and returns the last element in the list (MUST NOT BE EMPTY)
     * Complexity: O(1) average, O(N) worst
     */
    value_type pop_back() noexcept
    {
        iterator itQueue { std::prev(mQueue.end()) };
        mLookup.erase(get_key(*itQueue)); // O(1)-O(n)
        value_type v { std::move(*itQueue) };
        mQueue.erase(itQueue); // O(1)
        return v;
    }

    /**
     * Emplaces a new element on the front of the list, removing any element with the same key
     * Complexity: O(1) average, O(N) worst
     */
    template<class... Args>
    void enqueue_front(const Key& key, Args&&... args) noexcept
    {
        erase(key); // erase existing // O(1)-O(n)
        mQueue.emplace_front(key, std::forward<Args>(args)...); // O(1)
        mLookup[key] = mQueue.begin();
    }

protected:
    ValueQueue mQueue;
    ValueLookup mLookup;
};

/** A std::unordered_map that also acts as an ordered queue (keeps insertion order) */
template<typename Key, typename Value>
class OrderedMap : public OrderedMapAnyEntry<Key, Value, std::pair<const Key, Value>, OrderedMap<Key, Value>>
{
public:
    using value_type = std::pair<const Key, Value>;
    inline Key get_key(const value_type& e){ return e.first; }
    inline Value get_value(const value_type& e){ return e.second; }
    using OrderedMapAnyEntry<Key, Value, std::pair<const Key, Value>, OrderedMap<Key, Value>>::OrderedMapAnyEntry;
};

/** 
 * A std::list that provides fast lookup using a hash map (must be unique values)
 * This is an OrderedMap but with Values as Keys, and the value_type is just the Value not a pair
 */
template<typename Value>
class HashedQueue : public OrderedMapAnyEntry<Value, Value, Value, HashedQueue<Value>>
{
public:
    using value_type = Value;
    inline Value get_key(const value_type& e) noexcept{ return e; }
    inline Value get_value(const value_type& e) noexcept{ return e; }
    using OrderedMapAnyEntry<Value, Value, Value, HashedQueue<Value>>::OrderedMapAnyEntry;
};

} // namespace Andromeda

#endif // LIBA2_ORDEREDMAP_H_

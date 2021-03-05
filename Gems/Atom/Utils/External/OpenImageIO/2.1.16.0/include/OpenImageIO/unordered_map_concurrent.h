// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/hash.h>
#include <OpenImageIO/thread.h>

OIIO_NAMESPACE_BEGIN


namespace pvt {

// SFINAE test for whether class T has method `iterator find(key,hash)`.
// As described here: https://www.bfilipek.com/2016/02/sfinae-followup.html
// clang-format off
template <typename T>
class has_find_with_hash {
    using key_type = typename T::key_type;
    using iterator_type = typename T::iterator;
    template <typename U>
      static constexpr std::false_type test(...) { return {}; }
    template <typename U>
      static constexpr auto test(U* u) ->
        typename std::is_same<iterator_type, decltype(u->find(key_type(), size_t(0)))>::type { return {}; }
public:
    static constexpr bool value = test<T>(nullptr);
};
// clang-format on

}  // namespace pvt


// Helper function: find_with_hash.
//
// Calls `map.find(key, hash)` if a method with that signature exists for
// the Map type, otherwise just calls `map.find(key)`.
//
// This lets us use unordered_map_concurrent with underlying bin map types
// that do (e.g., robin_map) or do not (e.g., std::unordered_map) support a
// find method taking a precomputed hash.
template<class Map, class Key,
         OIIO_ENABLE_IF(pvt::has_find_with_hash<Map>::value)>
typename Map::iterator
find_with_hash(Map& map, const Key& key, size_t hash)
{
    return map.find(key, hash);
}

template<class Map, class Key,
         OIIO_ENABLE_IF(!pvt::has_find_with_hash<Map>::value)>
typename Map::iterator
find_with_hash(Map& map, const Key& key, size_t hash)
{
    return map.find(key);
}



/// unordered_map_concurrent provides an unordered_map replacement that
/// is optimized for concurrent access.  Its principle of operation is
/// similar to Java's ConcurrentHashMap.
///
/// With naive use of an unordered_map, multiple threads would have to
/// lock a mutex of some kind to control access to the map, either with
/// a unique full lock, or with a reader/writer lock.  But as the number
/// of threads contending for this shared resource rises, they end up
/// locking each other out and the map becomes a thread bottleneck.
///
/// unordered_map_concurrent solves this problem by internally splitting
/// the hash map into several disjoint bins, each of which is a standard
/// unordered_map.  For any given map item, the hash of its key
/// determines both the bin as well as its hashing within the bin (i.e.,
/// we break a big hash map into lots of little hash maps,
/// deterministically determined by the key).  Thus, we should expect
/// map entries to be spread more or less evenly among the bins.  There
/// is no mutex that locks the map as a whole; instead, each bin is
/// locked individually.  If the number of bins is larger than the
/// typical number of threads, most of the time two (or more) threads
/// accessing the map simultaneously will not be accessing the same bin,
/// and therefore will not be contending for the same lock.
///
/// unordered_map_concurrent provides an iterator which points to an
/// entry in the map and also knows which bin it is in and implicitly
/// holds a lock on the bin.  When the iterator is destroyed, the lock
/// on that bin is released.  When the iterator is incremented in such a
/// way that it transitions from the last entry of its current bin to
/// the first entry of the next bin, it will also release its current
/// lock and obtain a lock on the next bin.
///

template<class KEY, class VALUE, class HASH = std::hash<KEY>,
         class PRED = std::equal_to<KEY>, size_t BINS = 16,
         class BINMAP = std::unordered_map<KEY, VALUE, HASH, PRED>>
class unordered_map_concurrent {
public:
    typedef BINMAP BinMap_t;
    typedef typename BINMAP::iterator BinMap_iterator_t;
    using key_type = KEY;

public:
    unordered_map_concurrent() { m_size = 0; }

    ~unordered_map_concurrent()
    {
        //        for (size_t i = 0;  i < BINS;  ++i)
        //            std::cout << "Bin " << i << ": " << m_bins[i].map.size() << "\n";
    }

    /// An unordered_map_concurrent::iterator points to a specific entry
    /// in the umc, and holds a lock to the bin the entry is in.
    class iterator {
    public:
        friend class unordered_map_concurrent<KEY, VALUE, HASH, PRED, BINS,
                                              BINMAP>;

    public:
        /// Construct an unordered_map_concurrent iterator that points
        /// to nothing.
        iterator(unordered_map_concurrent* umc = NULL)
            : m_umc(umc)
            , m_bin(-1)
            , m_locked(false)
        {
        }

        /// Copy constructor of an unordered_map_concurrent iterator
        /// transfers the lock (if held) to this.  Caveat: the copied
        /// iterator no longer holds the lock!
        iterator(const iterator& src)
        {
            m_umc         = src.m_umc;
            m_bin         = src.m_bin;
            m_biniterator = src.m_biniterator;
            m_locked      = src.m_locked;
            // assignment transfers lock ownership
            *(const_cast<bool*>(&src.m_locked)) = false;
        }

        /// Destroying an unordered_map_concurrent iterator releases any
        /// bin locks it held.
        ~iterator() { clear(); }

        /// Totally invalidate this iterator -- point it to nothing
        /// (releasing any locks it may have had).
        void clear()
        {
            if (m_umc) {
                unbin();
                m_umc = NULL;
            }
        }

        // Dereferencing returns a reference to the hash table entry the
        // iterator refers to.
        const typename BinMap_t::value_type& operator*() const
        {
            return *m_biniterator;
        }

        /// Dereferencing returns a reference to the hash table entry the
        /// iterator refers to.
        const typename BinMap_t::value_type* operator->() const
        {
            return &(*m_biniterator);
        }

        /// Treating an iterator as a bool yields true if it points to a
        /// valid element of one of the bins of the map, false if it's
        /// equivalent to the end() iterator.
        operator bool()
        {
            return m_umc && m_bin >= 0
                   && m_biniterator != m_umc->m_bins[m_bin].map.end();
        }

        /// Iterator assignment transfers ownership of any bin locks
        /// held by the operand.
        iterator& operator=(const iterator& src)
        {
            unbin();
            m_umc         = src.m_umc;
            m_bin         = src.m_bin;
            m_biniterator = src.m_biniterator;
            m_locked      = src.m_locked;
            // assignment transfers lock ownership
            *(const_cast<bool*>(&src.m_locked)) = false;
            return *this;
        }

        bool operator==(const iterator& other) const
        {
            if (m_umc != other.m_umc)
                return false;
            if (m_bin == -1 && other.m_bin == -1)
                return true;
            return m_bin == other.m_bin && m_biniterator == other.m_biniterator;
        }
        bool operator!=(const iterator& other) { return !(*this == other); }

        /// Increment to the next entry in the map.  If we finish the
        /// bin we're in, move on to the next bin (releasing our lock on
        /// the old bin and acquiring a lock on the new bin).  If we
        /// finish the last bin of the map, return the end() iterator.
        void operator++()
        {
            DASSERT(m_umc);
            DASSERT(m_bin >= 0);
            ++m_biniterator;
            while (m_biniterator == m_umc->m_bins[m_bin].map.end()) {
                if (m_bin == BINS - 1) {
                    // ran off the end
                    unbin();
                    return;
                }
                rebin(m_bin + 1);
            }
        }
        void operator++(int) { ++(*this); }

        /// Lock the bin we point to, if not already locked.
        void lock()
        {
            if (m_bin >= 0 && !m_locked) {
                m_umc->m_bins[m_bin].lock();
                m_locked = true;
            }
        }
        /// Unlock the bin we point to, if locked.
        void unlock()
        {
            if (m_bin >= 0 && m_locked) {
                m_umc->m_bins[m_bin].unlock();
                m_locked = false;
            }
        }

        /// Without changing the lock status (i.e., the caller already
        /// holds the lock on the iterator's bin), increment to the next
        /// element within the bin.  Return true if it's pointing to a
        /// valid element afterwards, false if it ran off the end of the
        /// bin contents.
        bool incr_no_lock()
        {
            ++m_biniterator;
            return (m_biniterator != m_umc->m_bins[m_bin].map.end());
        }

    private:
        // No longer refer to a particular bin, release lock on the bin
        // it had (if any).
        void unbin()
        {
            if (m_bin >= 0) {
                if (m_locked)
                    unlock();
                m_bin = -1;
            }
        }

        // Point this iterator to a different bin, releasing locks on
        // the bin it previously referred to.
        void rebin(int newbin)
        {
            DASSERT(m_umc);
            unbin();
            m_bin = newbin;
            lock();
            m_biniterator = m_umc->m_bins[m_bin].map.begin();
        }

        unordered_map_concurrent* m_umc;  // which umc this iterator refers to
        int m_bin;                        // which bin within the umc
        BinMap_iterator_t m_biniterator;  // which entry within the bin
        bool m_locked;                    // do we own the lock on the bin?
    };


    /// Return an interator pointing to the first entry in the map.
    iterator begin()
    {
        iterator i(this);
        i.rebin(0);
        while (i.m_biniterator == m_bins[i.m_bin].map.end()) {
            if (i.m_bin == BINS - 1) {
                // ran off the end
                i.unbin();
                return i;
            }
            i.rebin(i.m_bin + 1);
        }
        return i;
    }

    /// Return an iterator signifying the end of the map (no valid
    /// entry pointed to).
    iterator end()
    {
        iterator i(this);
        return i;
    }

    /// Search for key.  If found, return an iterator referring to the
    /// element, otherwise, return an iterator that is equivalent to
    /// this->end().  If do_lock is true, lock the bin that we're
    /// searching and return the iterator in a locked state, and unlock
    /// the bin again if not found; however, if do_lock is false, assume
    /// that the caller already has the bin locked, so do no locking or
    /// unlocking and return an iterator that is unaware that it holds a
    /// lock.
    iterator find(const KEY& key, bool do_lock = true)
    {
        size_t hash = m_hash(key);
        size_t b    = whichbin(hash);
        Bin& bin(m_bins[b]);
        if (do_lock)
            bin.lock();
        auto it = find_with_hash(bin.map, key, hash);
        if (it == bin.map.end()) {
            // not found -- return the 'end' iterator
            if (do_lock)
                bin.unlock();
            return end();
        }
        // Found
        iterator i(this);
        i.m_bin         = (unsigned)b;
        i.m_biniterator = it;
        i.m_locked      = do_lock;
        return i;
    }

    /// Search for key. If found, return true and store the value. If not
    /// found, return false and do not alter value. If do_lock is true,
    /// read-lock the bin while we're searching, and release it before
    /// returning; however, if do_lock is false, assume that the caller
    /// already has the bin locked, so do no locking or unlocking.
    bool retrieve(const KEY& key, VALUE& value, bool do_lock = true)
    {
        size_t hash = m_hash(key);
        size_t b    = whichbin(hash);
        Bin& bin(m_bins[b]);
        if (do_lock)
            bin.lock();
        auto it    = find_with_hash(bin.map, key, hash);
        bool found = (it != bin.map.end());
        if (found)
            value = it->second;
        if (do_lock)
            bin.unlock();
        return found;
    }

    /// Insert <key,value> into the hash map if it's not already there.
    /// Return true if added, false if it was already present.
    /// If do_lock is true, lock the bin containing key while doing this
    /// operation; if do_lock is false, assume that the caller already
    /// has the bin locked, so do no locking or unlocking.
    bool insert(const KEY& key, const VALUE& value, bool do_lock = true)
    {
        size_t hash = m_hash(key);
        size_t b    = whichbin(hash);
        Bin& bin(m_bins[b]);
        if (do_lock)
            bin.lock();
        auto result = bin.map.emplace(key, value);
        if (result.second) {
            // the insert was succesful!
            ++m_size;
        }
        if (do_lock)
            bin.unlock();
        return result.second;
    }

    /// If the key is in the map, safely erase it.
    /// If do_lock is true, lock the bin containing key while doing this
    /// operation; if do_lock is false, assume that the caller already
    /// has the bin locked, so do no locking or unlocking.
    void erase(const KEY& key, bool do_lock = true)
    {
        size_t hash = m_hash(key);
        size_t b    = whichbin(hash);
        Bin& bin(m_bins[b]);
        if (do_lock)
            bin.lock();
        bin.map.erase(key, hash);
        if (do_lock)
            bin.unlock();
    }

    /// Return true if the entire map is empty.
    bool empty() { return m_size == 0; }

    /// Return the total number of entries in the map.
    size_t size() { return size_t(m_size); }

    /// Expliticly lock the bin that will contain the key (regardless of
    /// whether there is such an entry in the map), and return its bin
    /// number.
    size_t lock_bin(const KEY& key)
    {
        size_t hash = m_hash(key);
        size_t b    = whichbin(hash);
        m_bins[b].lock();
        return b;
    }

    /// Explicitly unlock the specified bin (this assumes that the caller
    /// holds the lock).
    void unlock_bin(size_t bin) { m_bins[bin].unlock(); }

private:
    struct Bin {
        OIIO_CACHE_ALIGN               // align bin to cache line
            mutable spin_mutex mutex;  // mutex for this bin
        BinMap_t map;                  // hash map for this bin
#ifndef NDEBUG
        mutable atomic_int m_nlocks;  // for debugging
#endif

        Bin()
        {
#ifndef NDEBUG
            m_nlocks = 0;
#endif
        }
        ~Bin()
        {
#ifndef NDEBUG
            DASSERT(m_nlocks == 0);
#endif
        }
        void lock() const
        {
            mutex.lock();
#ifndef NDEBUG
            ++m_nlocks;
            DASSERT_MSG(m_nlocks == 1, "oops, m_nlocks = %d", (int)m_nlocks);
#endif
        }
        void unlock() const
        {
#ifndef NDEBUG
            DASSERT_MSG(m_nlocks == 1, "oops, m_nlocks = %d", (int)m_nlocks);
            --m_nlocks;
#endif
            mutex.unlock();
        }
    };

    HASH m_hash;        // hashing function
    atomic_int m_size;  // total entries in all bins
    Bin m_bins[BINS];   // the bins

    static constexpr int log2(unsigned n)
    {
        return n < 2 ? 0 : 1 + log2(n / 2);
    }

    // Which bin will this key always appear in?
    size_t whichbin(size_t hash)
    {
        constexpr int LOG2_BINS = log2(BINS);
        constexpr int BIN_SHIFT = 8 * sizeof(size_t) - LOG2_BINS;

        static_assert(1 << LOG2_BINS == BINS,
                      "Number of bins must be a power of two");
        static_assert(~size_t(0) >> BIN_SHIFT == (BINS - 1), "Hash overflow");

        // Use the high order bits of the hash to index the bin. We assume that the
        // low-order bits of the hash will directly be used to index the hash table,
        // so using those would lead to collisions.
        size_t bin = hash >> BIN_SHIFT;
        DASSERT(bin < BINS);
        return bin;
    }
};


OIIO_NAMESPACE_END

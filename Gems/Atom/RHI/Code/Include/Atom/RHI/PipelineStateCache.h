/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/Utils/TypeHash.h>

namespace UnitTest
{
    class PipelineStateTests;
    class MultiDevicePipelineStateTests;
}

namespace AZ::RHI
{
    using PipelineStateHash = HashValue64;

    //! Used for storing a PipelineState object in a hash table structure (set, map, etc)
    struct PipelineStateEntry
    {
        PipelineStateEntry(
            PipelineStateHash hash, ConstPtr<PipelineState> pipelineState, const PipelineStateDescriptor& descriptor);

        bool operator < (const PipelineStateEntry& rhs) const
        {
            return m_hash < rhs.m_hash;
        }

        bool operator == (const PipelineStateEntry& rhs) const;

        //! Hash of the PipelineStateDescriptor
        PipelineStateHash m_hash;
        //! Pipeline state
        ConstPtr<PipelineState> m_pipelineState;

        //! Pipeline state descriptor variant for dispatch, draw, and ray tracing
        using PipelineStateDescriptorVariant = AZStd::variant<AZ::RHI::PipelineStateDescriptorForDraw, AZ::RHI::PipelineStateDescriptorForDispatch, AZ::RHI::PipelineStateDescriptorForRayTracing>;
        PipelineStateDescriptorVariant m_pipelineStateDescriptorVariant;
    };

    //! Hash calculator for a PipelineStateEntry
    struct PipelineStateCacheHash : public ::AZStd::hash<AZ::RHI::PipelineStateEntry>
    {
        AZStd::hash<AZ::RHI::PipelineStateEntry>::result_type operator () (const AZ::RHI::PipelineStateEntry& pipelineStateEntry) const
        {
            return static_cast<AZStd::hash<PipelineStateEntry>::result_type>(pipelineStateEntry.m_hash);
        }
    };

    //! The pipeline state set is an unordered set to help with detecting hash collisions and also faster find and store operations.
    using PipelineStateSet = AZStd::unordered_set<PipelineStateEntry, PipelineStateCacheHash>;

    //! Problem: High-level rendering code works in 'materials', 'shaders', and 'models', but the RHI works in
    //! 'pipeline states'. Therefore, a translation process must exist to resolve a shader variation (plus runtime
    //! state) into a pipeline state suitable for consumption by the RHI. These resolve operations can number in the
    //! thousands per frame, and (ideally) are heavily jobified.
    //!
    //! Another problem is that pipeline state creation is not fast, as on some platforms it will involve synchronous
    //! byte-code compilation. This could take anywhere from <1ms to >150ms. If compilation is done synchronously and
    //! immediately, the cache will effectively stall the entire process if multiple threads request the same pending
    //! pipeline state.
    //!
    //! Therefore, PipelineStateCache adheres to the following requirements:
    //!      1. A cache miss does not serialize all threads on a pipeline state compilation event.
    //!      2. A cache hit results in zero contention.
    //!
    //! Justification: Most pipeline state compilation will occur in the first few frames, but can also occur when new
    //! 'permutations' are hit while exploring. In the 90% case, the cache is warm and each frame results in a 100%
    //! cache hit rate. With zero locks, this scales extremely well across threads and removes a bottleneck from the
    //! render code. In the event that compilations are required, multiple threads are now able to participate in the
    //! compilation process without serializing each other.
    //!
    //! To accomplish this, the pipeline state cache uses three 'phases' of caching.
    //!      1. A global, read-only cache - designed as the 'fast path' for when the cache is warm.
    //!      2. A thread-local cache - reduces contention on the global pending cache for successive requests on the same thread.
    //!      3. A global, locked pending cache - de-duplicates pipeline state allocations.
    //!
    //! Each library has global and thread-local caches. Initially, the global cache is checked, if that fails, the
    //! thread-local cache is checked (no locks taken). Finally, the pending cache is checked under a lock and if
    //! the entry still doesn't exist, it is allocated and added to the pending cache. A thread-local PipelineLibrary
    //! is used to compile the pipeline state, which eliminates all locking for compilation.
    //!
    //! Pipeline states can be acquired at any time and from any thread. The cache will take a reader lock. During
    //! AcquirePipelineState, the global read-only cache is not updated, but the thread-local cache and pending
    //! global cache may be. Furthermore, compilations are performed on the calling thread, which means that separate
    //! thread may return a pipeline state that is still compiling. It is required that all pending AcquirePipelineState
    //! calls complete prior to using the returned pipeline state pointers during command list recording.
    //!
    //! Example Scenarios:
    //!
    //!  1. Threads request the same un-cached pipeline state:
    //!
    //!      Both the global read-only cache and thread-local caches miss, one thread wins the race to take a lock
    //!      on the global pending cache. It allocates but does not compile the pipeline state. All other threads wait on the
    //!      lock (which should be quick) and then find and return the uninitialized pipeline state. The compiling
    //!      thread uses the thread-local PipelineLibrary instance to compile the pipeline state. Non-compiling threads
    //!      will enter the uninitialized pipeline state into their thread-local cache (as does the compiling thread once it
    //!      completes). Note that the compiling thread is now busy, but all remaining threads are now unblocked to compile other
    //!      pipeline states.
    //!
    //!  2. A thread requests a pipeline state being compiled on another thread:
    //!
    //!      In this case, the global read-only cache won't have the pipeline state (since it's being compiled during
    //!      the current cycle, and the pending cache is only merged at the end of the cycle). It also won't have the
    //!      entry in the thread-local cache. It then hits the global pending cache, which will return the live instance
    //!      (being compiled). It then caches the result in its thread-local cache, so that successive requests will no
    //!      longer require a lock on the pending cache.
    //!
    //!  3. The cache is warm and all pipeline states are compiled:
    //!
    //!      Each thread hits the same read-only cache (which succeeds) and returns the pipeline state immediately.
    //!      This is the fast-path case where multiple threads are now able to resolve pipeline states with very
    //!      little performance overhead.
    //!
    //! Example Usage:
    //! @code{.cpp}
    //!      // Create library instance.
    //!      RHI::PipelineLibraryHandle libraryHandle = pipelineStateCache->CreateLibrary(serializedData); // Initial data loaded from disk.
    //!
    //!      // In jobs. Lots and lots of requests.
    //!      const RHI::PipelineState* pipelineState = pipelineStateCache->AcquirePipelineState(libraryHandle, descriptor);
    //!
    //!      // Reset contents of library. Releases all pipeline state references. Library remains valid.
    //!      pipelineStateCache->ResetLibrary(libraryHandle);
    //!
    //!      // Release library and all held references.
    //!      pipelineStateCache->ReleaseLibrary(libraryHandle);
    //! @endcode
    //!
    class PipelineStateCache final
        : public AZStd::intrusive_base
    {
        friend class UnitTest::MultiDevicePipelineStateTests;
    public:
        AZ_CLASS_ALLOCATOR(PipelineStateCache, SystemAllocator);

        //! The maximum number of libraries is configurable at compile time. A fixed number is used
        //! to avoid having to lazily resize thread-local arrays when traversing them, and also to
        //! avoid a pointer indirection on access.
        static const size_t LibraryCountMax = 256;

        static Ptr<PipelineStateCache> Create(MultiDevice::DeviceMask deviceMask);

        //! Resets the caches of all pipeline libraries back to empty. All internal references to pipeline states are released.
        void Reset();

        //! Creates an internal pipeline library instance and returns its handle.
        PipelineLibraryHandle CreateLibrary(
            const AZStd::unordered_map<int, ConstPtr<RHI::PipelineLibraryData>>& serializedData,
            const AZStd::unordered_map<int, AZStd::string>& filePaths);

        //! Releases the pipeline library and purges it from the cache. Releases all held references to pipeline states for the library.
        void ReleaseLibrary(PipelineLibraryHandle handle);

        //! Resets cache contents in the library. Releases all held references to pipeline states for the library.
        void ResetLibrary(PipelineLibraryHandle handle);

        //! Returns the resulting merged library from all the threadLibraries related to the passed in handle.
        //! The merged library can be used to write out the serialized data.
        Ptr<PipelineLibrary> GetMergedLibrary(PipelineLibraryHandle handle) const;

        //! Acquires a pipeline state (either draw or dispatch variants) from the cache. Pipeline states are associated
        //! to a specific library handle. Successive calls with the same pipeline state descriptor hash will return the same
        //! pipeline state, even across threads. If the library handle is invalid or the acquire operation fails, a null pointer
        //! is returned. Otherwise, a valid pipeline state pointer is returned (regardless of whether pipeline state compilation succeeds).
        //!
        //! It is permitted to take a strong reference to the returned pointer, but is not necessary as long as the reference
        //! is discarded on a library reset / release event. The cache will store a reference internally. If a strong reference
        //! is held externally, the instance will remain valid even after the cache is reset / destroyed.
        const PipelineState* AcquirePipelineState(
            PipelineLibraryHandle library, const PipelineStateDescriptor& descriptor, const AZ::Name& name = AZ::Name());

        //! This method merges the global pending cache into the global read-only cache and clears all thread-local caches.
        //! This reduces the total memory footprint of the caches and optimizes subsequent fetches. This method should be called
        //! once per frame.
        void Compact();

    private:
        PipelineStateCache(MultiDevice::DeviceMask deviceMask);

        void ValidateCacheIntegrity() const;

        struct GlobalLibraryEntry
        {
            // The global, read-only pipeline state set.
            PipelineStateSet m_readOnlyCache;

            // A global, locked cache used to de-duplicate pipeline allocations / compilations.
            PipelineStateSet m_pendingCache;
            AZStd::mutex m_pendingCacheMutex;

            // Tracks the number of pipeline states actively being compiled across all threads.
            AZStd::atomic_uint32_t m_pendingCompileCount = {0};

            // Contains the initial serialized data (Used to prime the thread libraries)
            // or the file name that contains the serialized data
            PipelineLibraryDescriptor m_pipelineLibraryDescriptor;
        };

        using GlobalLibrarySet = AZStd::fixed_vector<GlobalLibraryEntry, LibraryCountMax>;

        struct ThreadLibraryEntry
        {
            // A thread-local cache used to reduce contention on the global pending cache.
            PipelineStateSet m_threadLocalCache;

            //! Each thread has its own pipeline library. This allows threads to cache disjoint
            //! pipeline states without locking. The libraries are coalesced into a single library
            //! during GetMergedLibrary. The library is lazily initialized on the thread
            //! and uses the initial serialized data passed in at creation time.
            Ptr<PipelineLibrary> m_library;
        };

        //! Each thread has its own list of pipeline library entries. The index maps 1-to-1 with GlobalLibrarySet.
        //! GlobalLibrarySet contains the total size of the array; whereas the ThreadLibrarySet is just an array.
        //! The size of the global set should be used when traversing the thread library entries.
        using ThreadLibrarySet = AZStd::array<ThreadLibraryEntry, LibraryCountMax>;

        //! Helper function which binary searches a pipeline state set looking for an entry which matches the requested descriptor.
        static const PipelineState* FindPipelineState(
            const PipelineStateSet& pipelineStateSet, const PipelineStateDescriptor& descriptor);

        //! Helper function which inserts an entry into the set. Returns true if the entry was inserted, or false is a duplicate entry existed.
        static bool InsertPipelineState(PipelineStateSet& pipelineStateSet, PipelineStateEntry pipelineStateEntry);

        //! Performs a pipeline state compilation on the global cache using the thread-local pipeline library.
        ConstPtr<PipelineState> CompilePipelineState(
            GlobalLibraryEntry& globalLibraryEntry,
            ThreadLibraryEntry& threadLibraryEntry,
            const PipelineStateDescriptor& pipelineStateDescriptor,
            PipelineStateHash pipelineStateHash,
            const AZ::Name& name);

        //! Resets the library without validating the handle or taking a lock.
        void ResetLibraryImpl(PipelineLibraryHandle handle);

        MultiDevice::DeviceMask m_deviceMask;

        /// Each thread owns a set of ThreadLibraryEntry elements. RHI::PipelineLibraryHandle is an
        /// index into the array.
        ThreadLocalContext<ThreadLibrarySet> m_threadLibrarySet;

        /// This mutex guards library creation / reset / deletion.
        mutable AZStd::shared_mutex m_mutex;

        /// The set of library entries. The RHI::PipelineLibraryHandle maps into this array.
        GlobalLibrarySet m_globalLibrarySet;

        /// Tracks whether the library at the bit index is active.
        AZStd::bitset<LibraryCountMax> m_globalLibraryActiveBits;

        /// The free list of handles. This free list is checked first before expanding the watermark in order
        /// to recycle slots in m_globalLibrarySet.
        AZStd::fixed_vector<PipelineLibraryHandle, LibraryCountMax> m_libraryFreeList;

        // Friends
        friend class UnitTest::PipelineStateTests;
    };
}

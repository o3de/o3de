/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_HEAPALLOCATOR_H
#define CRYINCLUDE_CRYCOMMON_HEAPALLOCATOR_H
#pragma once


#include "Synchronization.h"
#include "Options.h"

#include <CrySizer.h>

//---------------------------------------------------------------------------
#define bMEM_ACCESS_CHECK       0
#define bMEM_HEAP_CHECK         0

namespace stl
{
    class HeapSysAllocator
    {
    public:
        static void* SysAlloc(size_t nSize)
        { return CryModuleMalloc(nSize); }
        static void SysDealloc(void* ptr)
        { CryModuleFree(ptr); }
    };

    class GlobalHeapSysAllocator
    {
    public:
        static void* SysAlloc(size_t nSize)
        {
            return CryModuleMalloc(nSize);
        }
        static void SysDealloc(void* ptr)
        {
            CryModuleFree(ptr);
        }
    };

    // Round up to next multiple of nAlign. Handles any positive integer.
    inline size_t RoundUpTo(size_t nSize, size_t nAlign)
    {
        assert(nAlign > 0);
        nSize += nAlign - 1;
        return nSize - nSize % nAlign;
    }

    /*---------------------------------------------------------------------------
    HeapAllocator
        A memory pool that can allocate arbitrary amounts of memory of arbitrary size
        and alignment. The heap may be freed all at once. Individual block deallocation
        is not provided.

        Usable as a base class to implement more general-purpose allocators that
        track, free, and reuse individual memory blocks.

        The class can optionally support multi-threading, using the second
        template parameter. By default it is multithread-safe.
        See Synchronization.h.

        Allocation details: Maintains a linked list of pages.
        All pages after first are in order of most free memory first.
        Allocations are from the smallest free page available.

    ---------------------------------------------------------------------------*/

    struct SMemoryUsage
    {
        size_t nAlloc, nUsed;

        SMemoryUsage(size_t _nAlloc = 0, size_t _nUsed = 0)
            : nAlloc(_nAlloc)
            , nUsed(_nUsed)
        {
            Validate();
        }

        size_t nFree() const
        {
            return nAlloc - nUsed;
        }
        void Validate() const
        {
            assert(nUsed <= nAlloc);
        }
        void Clear()
        {
            nAlloc = nUsed = 0;
        }

        void operator += (SMemoryUsage const& op)
        {
            nAlloc += op.nAlloc;
            nUsed += op.nUsed;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    struct FHeap
    {
        OPT_STRUCT(FHeap)
        OPT_VAR(size_t, PageSize);          // Pages allocated at this size, or multiple thereof if needed.
        OPT_VAR(bool, SinglePage)               // Only 1 page allowed (fixed alloc)
        OPT_VAR(bool, FreeWhenEmpty)        // Release all memory when no longer used
    };

    template <typename L = PSyncMultiThread, typename SysAl = HeapSysAllocator>
    class HeapAllocator
        : public FHeap
        , public L
        , private SysAl
    {
    public:

        typedef AutoLock<L> Lock;

        enum
        {
            DefaultAlignment = sizeof(void*)
        };
        enum
        {
            DefaultPageSize = 0x1000
        };

    private:

        struct PageNode
        {
            PageNode* pNext;
            char*           pEndAlloc;
            char*           pEndUsed;

            char* StartUsed() const
            {
                return (char*)(this + 1);
            }

            PageNode(size_t nAlloc)
            {
                pNext = 0;
                pEndAlloc = (char*)this + nAlloc;
                pEndUsed = StartUsed();
            }

            void* Allocate(size_t nSize, size_t nAlign)
            {
                // Align current mem.
                char* pNew = Align(pEndUsed, nAlign);
                if (pNew + nSize > pEndAlloc)
                {
                    return 0;
                }
                pEndUsed = pNew + nSize;
                return pNew;
            }

            bool CanAllocate(size_t nSize, size_t nAlign)
            {
                return Align(pEndUsed, nAlign) + nSize <= pEndAlloc;
            }

            void Reset()
            {
                pEndUsed = StartUsed();
            }

            size_t GetMemoryAlloc() const
            {
                return pEndAlloc - (char*)this;
            }
            size_t GetMemoryUsed() const
            {
                return pEndUsed - StartUsed();
            }
            size_t GetMemoryFree() const
            {
                return pEndAlloc - pEndUsed;
            }

            void Validate() const
            {
                assert(pEndAlloc >= (char*)this);
                assert(pEndUsed >= StartUsed() && pEndUsed <= pEndAlloc);
            }

            bool CheckPtr(void* ptr) const
            {
                return (char*)ptr >= StartUsed() && (char*)ptr < pEndUsed;
            }
        };

    public:

        HeapAllocator(FHeap opts = 0)
            : FHeap(opts)
            , _pPageList(0)
        {
            PageSize = max<size_t>(Align(PageSize, DefaultPageSize), DefaultPageSize);
        }

        ~HeapAllocator()
        {
            Clear();
        }

        //
        // Raw memory allocation.
        //
        void* Allocate(const Lock& lock, size_t nSize, size_t nAlign = DefaultAlignment)
        {
            for (;; )
            {
                // Try allocating from head page first.
                if (_pPageList)
                {
                    if (void* ptr = _pPageList->Allocate(nSize, nAlign))
                    {
                        _TotalMem.nUsed += nSize;
                        return ptr;
                    }

                    if (_pPageList->pNext && _pPageList->pNext->GetMemoryFree() > _pPageList->GetMemoryFree())
                    {
                        SortPage(lock, _pPageList);
                        Validate(lock);

                        // Try allocating from new head, which has the most free memory.
                        // If this fails, we know no further pages will succeed.
                        if (void* ptr = _pPageList->Allocate(nSize, nAlign))
                        {
                            _TotalMem.nUsed += nSize;
                            return ptr;
                        }
                    }
                    if (SinglePage)
                    {
                        return 0;
                    }
                }

                // Allocate the new page of the required size.
                size_t nAllocSize = Align(sizeof(PageNode), nAlign) + nSize;
                nAllocSize = RoundUpTo(nAllocSize, PageSize);

                void* pAlloc = this->SysAlloc(nAllocSize);
                PageNode* pPageNode = new(pAlloc) PageNode(nAllocSize);

                // Insert at head of list.
                pPageNode->pNext = _pPageList;
                _pPageList = pPageNode;

                _TotalMem.nAlloc += nAllocSize;

                Validate(lock);
            }
        }

        void Deallocate([[maybe_unused]] const Lock& lock, [[maybe_unused]] void* ptr, size_t nSize)
        {
            // Just to maintain counts, can't reuse memory.
            assert(CheckPtr(lock, ptr));
            assert(_TotalMem.nUsed >= nSize);
            _TotalMem.nUsed -= nSize;
        }

        //
        // Templated type allocation.
        //
        template<typename T>
        T* New(size_t nAlign = 0)
        {
            void* pMemory = Allocate(Lock(*this), sizeof(T), nAlign ? nAlign : alignof(T));
            return pMemory ? new(pMemory) T : 0;
        }

        template<typename T>
        T* NewArray(size_t nCount, size_t nAlign = 0)
        {
            void* pMemory = Allocate(Lock(*this), sizeof(T) * nCount, nAlign ? nAlign : alignof(T));
            return pMemory ? new(pMemory) T[nCount] : 0;
        }

        //
        // Maintenance.
        //
        SMemoryUsage GetTotalMemory(const Lock&)
        {
            return _TotalMem;
        }
        SMemoryUsage GetTotalMemory()
        {
            Lock lock(*this);
            return _TotalMem;
        }

        // Facility to defer freeing of dead pages during memory release calls.
        struct FreeMemLock
            : Lock
        {
            struct PageNode* _pPageList;

            FreeMemLock(L& lock)
                : Lock(lock)
                , _pPageList(0) {}

            ~FreeMemLock()
            {
                while (_pPageList != 0)
                {
                    // Read the "next" pointer before deleting.
                    PageNode* pNext = _pPageList->pNext;

                    // Delete the current page.
                    SysAl::SysDealloc(_pPageList);

                    // Move to the next page in the list.
                    _pPageList = pNext;
                }
            }
        };

        void Clear(FreeMemLock& lock)
        {
            // Remove the pages from the object.
            Validate(lock);
            lock._pPageList = _pPageList;
            _pPageList = 0;
            _TotalMem.Clear();
        }

        void Clear()
        {
            FreeMemLock lock(*this);
            Clear(lock);
        }

        void Reset(const Lock& lock)
        {
            // Reset all pages, allowing memory re-use.
            Validate(lock);
            size_t nPrevSize = ~0;
            for (PageNode** ppPage = &_pPageList; *ppPage; )
            {
                (*ppPage)->Reset();
                if ((*ppPage)->GetMemoryAlloc() > nPrevSize)
                {
                    // Move page to sorted location near beginning.
                    SortPage(lock, *ppPage);

                    // ppPage is now next page, so continue loop.
                    continue;
                }
                nPrevSize = (*ppPage)->GetMemoryAlloc();
                ppPage = &(*ppPage)->pNext;
            }
            _TotalMem.nUsed = 0;
            Validate(lock);
        }

        void Reset()
        {
            Reset(Lock(*this));
        }

        //
        // Validation.
        //
        bool CheckPtr(const Lock&, void* ptr) const
        {
            if (!ptr)
            {
                return true;
            }
            for (PageNode* pNode = _pPageList; pNode; pNode = pNode->pNext)
            {
                if (pNode->CheckPtr(ptr))
                {
                    return true;
                }
            }
            return false;
        }

        void Validate(const Lock&) const
        {
        #ifdef _DEBUG
            // Check page validity, and memory counts.
            SMemoryUsage MemCheck;

            for (PageNode* pPage = _pPageList; pPage; pPage = pPage->pNext)
            {
                pPage->Validate();
                if (pPage != _pPageList && pPage->pNext)
                {
                    assert(pPage->GetMemoryFree() >= pPage->pNext->GetMemoryFree());
                }
                MemCheck.nAlloc += pPage->GetMemoryAlloc();
                MemCheck.nUsed += pPage->GetMemoryUsed();
            }
            assert(MemCheck.nAlloc == _TotalMem.nAlloc);
            assert(MemCheck.nUsed >= _TotalMem.nUsed);
        #endif

        #if bMEM_HEAP_CHECK
            static int nCount = 0, nInterval = 0;
            if (nCount++ >= nInterval)
            {
                nInterval++;
                nCount = 0;
            }
        #endif
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            Lock lock(non_const(*this));
            for (PageNode* pNode = _pPageList; pNode; pNode = pNode->pNext)
            {
                pSizer->AddObject(pNode, pNode->GetMemoryAlloc());
            }
        }

    private:

        void SortPage(const Lock&, PageNode*& rpPage)
        {
            // Unlink rpPage.
            PageNode* pPage = rpPage;
            rpPage = pPage->pNext;

            // Insert into list based on free memory.
            PageNode** ppBefore = &_pPageList;
            while (*ppBefore && (*ppBefore)->GetMemoryFree() > pPage->GetMemoryFree())
            {
                ppBefore = &(*ppBefore)->pNext;
            }

            // Link before rpList.
            pPage->pNext = *ppBefore;
            *ppBefore = pPage;
        }

        PageNode*           _pPageList;     // All allocated pages.
        SMemoryUsage    _TotalMem;      // Track memory allocated and used.
    };
}

#endif // CRYINCLUDE_CRYCOMMON_HEAPALLOCATOR_H

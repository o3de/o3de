/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYPOOL_STLWRAPPER_H
#define CRYINCLUDE_CRYPOOL_STLWRAPPER_H
#pragma once

namespace NCryPoolAlloc
{
    //namespace CSTLPoolAllocWrapperHelper
    //{
    //  inline void destruct(char *) {}
    //  inline void destruct(wchar_t*) {}
    //  template <typename T>
    //      inline void destruct(T *t) {t->~T();}
    //}

    //template <size_t S, class L, size_t A, typename T>
    //struct CSTLPoolAllocWrapperStatic
    //{
    //  static PoolAllocator<S, L, A> * allocator;
    //};

    //template <class T, class L, size_t A>
    //struct CSTLPoolAllocWrapperKungFu : public CSTLPoolAllocWrapperStatic<sizeof(T),L,A,T>
    //{
    //};

    template <class T, class TCont>
    class CSTLPoolAllocWrapper
    {
    private:
        static TCont*           m_pContainer;
    public:
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef T*        pointer;
        typedef const T*  const_pointer;
        typedef T&        reference;
        typedef const T&  const_reference;
        typedef T         value_type;

        static TCont*           Container(){return m_pContainer; }
        static void             Container(TCont* pContainer){m_pContainer = pContainer; }


        template <class U>
        struct rebind
        {
            typedef CSTLPoolAllocWrapper<T, TCont> other;
        };

        CSTLPoolAllocWrapper() throw()
        {
        }

        CSTLPoolAllocWrapper(const CSTLPoolAllocWrapper&) throw()
        {
        }

        template <class TTemp, class TTempCont>
        CSTLPoolAllocWrapper(const CSTLPoolAllocWrapper<TTemp, TTempCont>&) throw()
        {
        }

        ~CSTLPoolAllocWrapper() throw()
        {
        }

        pointer address(reference x) const
        {
            return &x;
        }

        const_pointer address(const_reference x) const
        {
            return &x;
        }

        pointer allocate(size_type n = 1, const_pointer hint = 0)
        {
            TCont* pContainer   =   Container();
            uint8* pData            =   pContainer->TCont::template Allocate<uint8*>(n * sizeof(T), sizeof(T));
            return pContainer->TCont::template Resolve<pointer>(pData);
            //  return Container()?Container()->Allocate<void*>(n*sizeof(T),sizeof(T)):0
        }

        void deallocate(pointer p, size_type n = 1)
        {
            if (Container())
            {
                Container()->Free(p);
            }
        }

        size_type max_size() const throw()
        {
            return Container() ? Container()->MemSize() : 0;
        }

        void construct(pointer p, const T& val)
        {
            new(static_cast<void*>(p))T(val);
        }

        void construct(pointer p)
        {
            new(static_cast<void*>(p))T();
        }

        void destroy(pointer p)
        {
            p->~T();
        }

        pointer new_pointer()
        {
            return new(allocate())T();
        }

        pointer new_pointer(const T& val)
        {
            return new(allocate())T(val);
        }

        void delete_pointer(pointer p)
        {
            p->~T();
            deallocate(p);
        }

        bool operator==(const CSTLPoolAllocWrapper&) {return true; }
        bool operator!=(const CSTLPoolAllocWrapper&) {return false; }
    };
}

#endif // CRYINCLUDE_CRYPOOL_STLWRAPPER_H


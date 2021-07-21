/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Declaration and definition of the CrySizer class, which is used to
//               calculate the memory usage by the subsystems and components, to help
//               the artists keep the memory budged low.


#ifndef CRYINCLUDE_CRYCOMMON_CRYSIZER_H
#define CRYINCLUDE_CRYCOMMON_CRYSIZER_H
#pragma once


//////////////////////////////////////////////////////////////////////////

// common containers for overloads
#include <list>

#include "Cry_Math.h"
#include <StlUtils.h>
#include <Tarray.h>
#include <CryPodArray.h>
#include <Cry_Vector3.h>
#include <Cry_Quat.h>
#include <Cry_Color.h>
#include <smartptr.h>

// forward declarations for overloads
struct AABB;
struct SVF_P3F;
struct SVF_P3F_C4B_T2F;
struct SVF_P3F_C4B_T2S;
struct SVF_P3S_C4B_T2S;
struct SPipTangents;

#ifdef WIN64
#include <string.h> // workaround for Amd64 compiler
#endif

namespace AZ
{
    class Vector3;
}

// flags applicable to the ICrySizer (retrieved via getFlags() method)
//
enum ICrySizerFlagsEnum
{
    // if this flag is set, during getSize(), the subsystem must count all the objects
    // it uses in the other subsystems also
    CSF_RecurseSubsystems = 1 << 0,

    CSF_Reserved1 = 1 << 1,
    CSF_Reserved2 = 1 << 2
};

//////////////////////////////////////////////////////////////////////////
// Helper functions to calculate size of the std containers.
//////////////////////////////////////////////////////////////////////////
namespace stl
{
    template <class Map>
    inline size_t size_of_map(const Map& m)
    {
        if (!m.empty())
        {
            return m.size() * sizeof(typename Map::value_type) + m.size() * sizeof(MapLikeStruct);
        }
        return 0;
    }
    template <class Map>
    inline size_t size_of_set(const Map& m)
    {
        if (!m.empty())
        {
            return m.size() * sizeof(typename Map::value_type) + m.size() * sizeof(MapLikeStruct);
        }
        return 0;
    }
    template <class List>
    inline size_t size_of_list(const List& c)
    {
        if (!c.empty())
        {
            return c.size() * sizeof(typename List::value_type) + c.size() * sizeof(void*) * 2; // sizeof stored type + 2 pointers prev,next
        }
        return 0;
    }
    template <class Deque>
    inline size_t size_of_deque(const Deque& c)
    {
        if (!c.empty())
        {
            return c.size() * sizeof(typename Deque::value_type);
        }
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////
// interface ICrySizer
// USAGE
//   An instance of this class is passed down to each and every component in the system.
//   Every component it's passed to optionally pushes its name on top of the
//     component name stack (thus ensuring that all the components calculated down
//     the tree will be assigned the correct subsystem/component name)
//   Every component must Add its size with one of the Add* functions, and Add the
//     size of all its subcomponents recursively
//   In order to push the component/system name on the name stack, the clients must
//     use the SIZER_COMPONENT_NAME macro or CrySizerComponentNameHelper class:
//
//   void X::getSize (ICrySizer* pSizer)
//   {
//     SIZER_COMPONENT_NAME(pSizer, X);
//     if (!pSizer->Add (this))
//       return;
//     pSizer->Add (m_arrMySimpleArray);
//     pSizer->Add (m_setMySimpleSet);
//     m_pSubobject->getSize (pSizer);
//   }
//
//   The Add* functions return bool. If they return true, then the object has been added
//   to the set for the first time, and you should go on recursively adding all its children.
//   If it returns false, then you can spare time and rather not go on into recursion;
//   however it doesn't reflect on the results: an object that's added more than once is
//   counted only once.
//
// WARNING:
//   If you have an array (pointer), you should Add its size with addArray
class ICrySizer
{
public:
    virtual ~ICrySizer(){}
    // this class is used to push/pop the name to/from the stack automatically
    // (to exclude stack overruns or underruns at runtime)
    friend class CrySizerComponentNameHelper;

    virtual void Release() = 0;

    // Return total calculated size.
    virtual size_t GetTotalSize() = 0;

    // Return total objects added.
    virtual size_t GetObjectCount() = 0;

    // Resets the counting.
    virtual void Reset() = 0;

    virtual void End() = 0;

    // adds an object identified by the unique pointer (it needs not be
    // the actual object position in the memory, though it would be nice,
    // but it must be unique throughout the system and unchanging for this object)
    // nCount parameter is only used for counting number of objects, it doesnt affect the size of the object.
    // RETURNS: true if the object has actually been added (for the first time)
    //          and calculated
    virtual bool AddObject (const void* pIdentifier, size_t nSizeBytes, int nCount = 1) = 0;

    template<typename Type>
    bool AddObjectSize(const Type* pObj)
    {
        return AddObject(pObj, sizeof *pObj);
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    // temp dummy function while checking in the CrySizer changes, will be removed soon

    template<typename Type>
    void AddObject(const Type& rObj)
    {
        (void)rObj;
    }

    template<typename Type>
    void AddObject(Type* pObj)
    {
        if (pObj)
        {
            //forward to reference object to allow function overload
            this->AddObject(*pObj);
        }
    }

    // overloads for smart_ptr and other common objects
    template<typename T>
    void AddObject(const _smart_ptr<T>& rObj) { this->AddObject(rObj.get()); }
    template<typename T>
    void AddObject(const AZStd::shared_ptr<T>& rObj) { this->AddObject(rObj.get()); }
    template<typename T>
    void AddObject(const std::shared_ptr<T>& rObj) { this->AddObject(rObj.get()); }
    template<typename T>
    void AddObject(const std::unique_ptr<T>& rObj) { this->AddObject(rObj.get()); }
    template<typename T, typename U>
    void AddObject(const std::pair<T, U>& rPair)
    {
        this->AddObject(rPair.first);
        this->AddObject(rPair.second);
    }
    template<typename T, typename U>
    void AddObject(const AZStd::pair<T, U>& rPair)
    {
        this->AddObject(rPair.first);
        this->AddObject(rPair.second);
    }
    void AddObject(const string& rString) {this->AddObject(rString.c_str(), rString.capacity()); }
    void AddObject(const CryStringT<wchar_t>& rString) {this->AddObject(rString.c_str(), rString.capacity()); }
    void AddObject(const CryFixedStringT<32>&){}
    void AddObject(const wchar_t&) {}
    void AddObject(const char&) {}
    void AddObject(const unsigned char&) {}
    void AddObject(const signed char&) {}
    void AddObject(const short&) {}
    void AddObject(const unsigned short&) {}
    void AddObject(const int&) {}
    void AddObject(const unsigned int&) {}
    void AddObject(const long&) {}
    void AddObject(const unsigned long&) {}
    void AddObject(const float&) {}
    void AddObject(const bool&) {}
    void AddObject(const unsigned long long&) {}
    void AddObject(const long long&) {}
    void AddObject(const double&) {}
    void AddObject(const Vec2&) {}
    void AddObject(const Vec3&) {}
    void AddObject(const Vec4&) {}
    void AddObject(const Ang3&) {}
    void AddObject(const Matrix34&) {}
    void AddObject(const Quat&) {}
    void AddObject(const QuatT&) {}
    void AddObject(const QuatTS&) {}
    void AddObject(const ColorF&) {}
    void AddObject(const AABB&) {}
    void AddObject(const SVF_P3F&) {}
    void AddObject(const SVF_P3F_C4B_T2F&) {}
    void AddObject(const SVF_P3F_C4B_T2S&) {}
    void AddObject(const SVF_P3S_C4B_T2S&) {}
    void AddObject(const SPipTangents&) {}
    void AddObject([[maybe_unused]] const AZ::Vector3& rObj) {}
    void AddObject(void*) {}

    // overloads for container, will automaticly traverse the content
    template<typename T, typename Alloc>
    void AddObject(const std::list<T, Alloc>& rList)
    {
        // dummy struct to get correct element size
        struct Dummy
        {
            void* a;
            void* b;
            T t;
        };

        for (typename std::list<T, Alloc>::const_iterator it = rList.begin(); it != rList.end(); ++it)
        {
            if (this->AddObject(&(*it), sizeof(Dummy)))
            {
                this->AddObject(*it);
            }
        }
    }
    template<typename K, typename T, typename Comp, typename Equal, typename Alloc>
    void AddObject([[maybe_unused]] const AZStd::unordered_map<K, T, Comp, Equal, Alloc>& rVector)
    {

    }

    template<typename T, typename Alloc>
    void AddObject(const std::vector<T, Alloc>& rVector)
    {
        if (rVector.empty())
        {
            this->AddObject(&rVector, rVector.capacity() * sizeof(T));
            return;
        }

        if (!this->AddObject(&rVector[0], rVector.capacity() * sizeof(T)))
        {
            return;
        }

        for (typename std::vector<T, Alloc>::const_iterator it = rVector.begin(); it != rVector.end(); ++it)
        {
            this->AddObject(*it);
        }
    }

    template<typename T, typename Alloc>
    void AddObject(const std::deque<T, Alloc>& rVector)
    {
        for (typename std::deque<T, Alloc>::const_iterator it = rVector.begin(); it != rVector.end(); ++it)
        {
            if (this->AddObject(&(*it), sizeof(T)))
            {
                this->AddObject(*it);
            }
        }
    }

    template<typename T, typename I, typename S>
    void AddObject(const DynArray<T, I, S>& rVector)
    {
        if (rVector.empty())
        {
            this->AddObject(rVector.begin(), rVector.get_alloc_size());
            return;
        }

        if (!this->AddObject(rVector.begin(), rVector.get_alloc_size()))
        {
            return;
        }

        for (typename DynArray<T, I, S>::const_iterator it = rVector.begin(); it != rVector.end(); ++it)
        {
            this->AddObject(*it);
        }
    }

    template<typename T>
    void AddObject(const PodArray<T>& rVector)
    {
        if (!this->AddObject(rVector.begin(), rVector.capacity() * sizeof(T)))
        {
            return;
        }

        for (typename PodArray<T>::const_iterator it = rVector.begin(); it != rVector.end(); ++it)
        {
            this->AddObject(*it);
        }
    }

    template<typename K, typename T, typename Comp, typename Alloc>
    void AddObject(const std::map<K, T, Comp, Alloc>& rVector)
    {
        // dummy struct to get correct element size
        struct Dummy
        {
            void* a;
            void* b;
            void* c;
            void* d;
            K k;
            T t;
        };

        for (typename std::map<K, T, Comp, Alloc>::const_iterator it = rVector.begin(); it != rVector.end(); ++it)
        {
            if (this->AddObject(&(*it), sizeof(Dummy)))
            {
                this->AddObject(it->first);
                this->AddObject(it->second);
            }
        }
    }

    template<typename T, typename Comp, typename Alloc>
    void AddObject(const std::set<T, Comp, Alloc>& rVector)
    {
        // dummy struct to get correct element size
        struct Dummy
        {
            void* a;
            void* b;
            void* c;
            void* d;
            T t;
        };

        for (typename std::set<T, Comp, Alloc>::const_iterator it = rVector.begin(); it != rVector.end(); ++it)
        {
            if (this->AddObject(&(*it), sizeof(Dummy)))
            {
                this->AddObject(*it);
            }
        }
    }

    template <typename TKey, typename TValue, typename TPredicate, typename TAlloc>
    void AddObject (const std::multimap<TKey, TValue, TPredicate, TAlloc>& rContainer)
    {
        AddContainer(rContainer);
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    bool Add (const T* pId, size_t num)
    {
        return AddObject(pId, num * sizeof(T));
    }

    template <class T>
    bool Add (const T& rObject)
    {
        return AddObject (&rObject, sizeof(T));
    }

    bool Add (const char* szText)
    {
        return AddObject(szText, strlen(szText) + 1);
    }

    template <class StringCls>
    bool AddString (const StringCls& strText)
    {
        if (!strText.empty())
        {
            return AddObject (strText.c_str(), strText.size());
        }
        else
        {
            return false;
        }
    }
#ifdef _XSTRING_
    template <class Elem, class Traits, class Allocator>
    bool Add (const std::basic_string<Elem, Traits, Allocator>& strText)
    {
        AddString (strText);
        return true;
    }
#endif

 #ifndef NOT_USE_CRY_STRING
    bool Add (const string& strText)
    {
        AddString(strText);
        return true;
    }
 #endif


    // Template helper function to add generic stl container
    template <typename Container>
    bool AddContainer (const Container& rContainer)
    {
        if (rContainer.capacity())
        {
            return AddObject (&rContainer, rContainer.capacity() * sizeof(typename Container::value_type));
        }
        return false;
    }
    template <typename Container>
    bool AddHashMap(const Container& rContainer)
    {
        if (!rContainer.empty())
        {
            return AddObject (&(*rContainer.begin()), rContainer.size() * sizeof(typename Container::value_type));
        }
        return false;
    }

    // Specialization of the AddContainer for the std::list
    template <typename Type, typename TAlloc>
    bool AddContainer (const std::list<Type, TAlloc>& rContainer)
    {
        if (!rContainer.empty())
        {
            return AddObject(&(*rContainer.begin()), stl::size_of_list(rContainer));
        }
        return false;
    }
    // Specialization of the AddContainer for the std::deque
    template <typename Type, typename TAlloc>
    bool AddContainer (const std::deque<Type, TAlloc>& rContainer)
    {
        if (!rContainer.empty())
        {
            return AddObject(&(*rContainer.begin()), stl::size_of_deque(rContainer));
        }
        return false;
    }
    // Specialization of the AddContainer for the std::map
    template <typename TKey, typename TValue, typename TPredicate, typename TAlloc>
    bool AddContainer (const std::map<TKey, TValue, TPredicate, TAlloc>& rContainer)
    {
        if (!rContainer.empty())
        {
            return AddObject(&(*rContainer.begin()), stl::size_of_map(rContainer));
        }
        return false;
    }
    // Specialization of the AddContainer for the std::multimap
    template <typename TKey, typename TValue, typename TPredicate, typename TAlloc>
    bool AddContainer (const std::multimap<TKey, TValue, TPredicate, TAlloc>& rContainer)
    {
        if (!rContainer.empty())
        {
            return AddObject(&(*rContainer.begin()), stl::size_of_map(rContainer));
        }
        return false;
    }
    // Specialization of the AddContainer for the std::set
    template <typename TKey, typename TPredicate, typename TAlloc>
    bool AddContainer (const std::set<TKey, TPredicate, TAlloc>& rContainer)
    {
        if (!rContainer.empty())
        {
            return AddObject(&(*rContainer.begin()), stl::size_of_set(rContainer));
        }
        return false;
    }

    // Specialization of the AddContainer for the AZStd::unordered_map
    template <typename KEY, typename TYPE, class HASH, class EQUAL, class ALLOCATOR>
    bool AddContainer(const AZStd::unordered_map<KEY, TYPE, HASH, EQUAL, ALLOCATOR>& rContainer)
    {
        if (!rContainer.empty())
        {
            return AddObject(&(*rContainer.begin()), rContainer.size() * sizeof(typename AZStd::unordered_map<KEY, TYPE, HASH, EQUAL, ALLOCATOR>::value_type));
        }
        else
        {
            return false;
        }
    }

    void Test()
    {
        std::map<int, float> mymap;
        AddContainer(mymap);
    }

    // returns the flags
    unsigned GetFlags() const {return m_nFlags; }

protected:
    // these functions must operate on the component name stack
    // they are to be only accessible from within class CrySizerComponentNameHelper
    // which should be used through macro SIZER_COMPONENT_NAME
    virtual void Push (const char* szComponentName) = 0;
    // pushes the name that is the name of the previous component . (dot) this name
    virtual void PushSubcomponent (const char* szSubcomponentName) = 0;
    virtual void Pop () = 0;

    unsigned m_nFlags;
};

//////////////////////////////////////////////////////////////////////////
// This is on-stack class that is only used to push/pop component names
// to/from the sizer name stack.
//
// USAGE:
//
//   Create an instance of this class at the start of a function, before
//   calling Add* methods of the sizer interface. Everything added in the
//   function and below will be considered this component, unless
//   explicitly set otherwise.
//
class CrySizerComponentNameHelper
{
public:
    // pushes the component name on top of the name stack of the given sizer
    CrySizerComponentNameHelper (ICrySizer* pSizer, const char* szComponentName, bool bSubcomponent)
        : m_pSizer(pSizer)
    {
        if (bSubcomponent)
        {
            pSizer->PushSubcomponent (szComponentName);
        }
        else
        {
            pSizer->Push (szComponentName);
        }
    }

    // pops the component name off top of the name stack of the sizer
    ~CrySizerComponentNameHelper()
    {
        m_pSizer->Pop();
    }

protected:
    ICrySizer* m_pSizer;
};

// use this to push (and automatically pop) the sizer component name at the beginning of the
// getSize() function
#define SIZER_COMPONENT_NAME(pSizerPointer, szComponentName) PREFAST_SUPPRESS_WARNING(6246) CrySizerComponentNameHelper AZ_JOIN(sizerHelper, __LINE__)(pSizerPointer, szComponentName, false)
#define SIZER_SUBCOMPONENT_NAME(pSizerPointer, szComponentName) PREFAST_SUPPRESS_WARNING(6246) CrySizerComponentNameHelper AZ_JOIN(sizerHelper, __LINE__)(pSizerPointer, szComponentName, true)

#endif // CRYINCLUDE_CRYCOMMON_CRYSIZER_H

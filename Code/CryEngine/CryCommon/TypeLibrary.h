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

#ifndef CRYINCLUDE_CRYCOMMON_TYPELIBRARY_H
#define CRYINCLUDE_CRYCOMMON_TYPELIBRARY_H
#pragma once

#include <ISoftCodeMgr.h> // <> required for Interfuscator

#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>

#ifdef SOFTCODE_ENABLED

// Internal: Used by SC types to auto-remove themselves from their TypeRegistrar on destruction.
struct InstanceTracker
{
    InstanceTracker()
        : m_pRegistrar()
    {}

    ~InstanceTracker()
    {
        if (m_pRegistrar)
        {
            m_pRegistrar->RemoveInstance(this);
        }
    }

    void SetRegistrar(ITypeRegistrar* pRegistrar)
    {
        m_pRegistrar = pRegistrar;
    }

    ITypeRegistrar* m_pRegistrar;       // Valid if created by a registrar, otherwise NULL
};

#endif


#ifdef SOFTCODE_ENABLED

// Include this for SEH support
    #include <excpt.h>

/*
    Used to declare the interface as having an associated TypeLibrary.
    Usage:
    struct IMyInterface
    {
        DECLARE_TYPELIB(IMyInterface);
*/
    #define DECLARE_TYPELIB(IName)               \
    static void VisitMembers(IExchanger & ex) {} \
    typedef CTypeLibrary<IName> TLibrary

/*
    Exposes a class to a TypeLibrary for registration.
    Usage:
    class MyThing : public IThing
    {
        DECLARE_TYPE(MyThing, IThing);
        ...
*/
    #define DECLARE_TYPE(TName, TSuperType)                                                                \
public:                                                                                                    \
    void VisitMembers(IExchanger & ex) { TSuperType::VisitMembers(ex); VisitMember<__START_MEMBERS>(ex); } \
private:                                                                                                   \
    friend class TypeRegistrar<TName>;                                                                     \
    static const size_t __START_MEMBERS = __COUNTER__ + 1;                                                 \
    template <size_t IDX>                                                                                  \
    void VisitMember(IExchanger & exchanger) {}                                                            \
    InstanceTracker __instanceTracker

    #ifdef SOFTCODE
        #define _EXPORT_TYPE_LIB(Interface) \
    extern "C" ITypeLibrary * GetTypeLibrary() { return CTypeLibrary<Interface>::Instance(); }
    #else
        #define _EXPORT_TYPE_LIB(Interface)
    #endif

// Internal: Outputs the specialized method template for the member at index
    #define _SOFT_MEMBER_VISITOR(member, index) \
    template <>                                 \
    void VisitMember<index>(IExchanger & ex) { ex.Visit(#member, member); VisitMember<index + 1>(ex); }

/*
    Used to expose a class member to SoftCoding (to allow run-time member exchange)
    If SoftCode is disabled this does nothing and simple emits the member.
    For array types, use SOFT_ARRAY() instead or use AZStd::array which allows assignment.
    Usage: std::vector<string> SOFT(m_myStrings);
*/
    #define SOFT(member) \
    member;              \
    _SOFT_MEMBER_VISITOR(member, __COUNTER__)

/*
    Used to expose a primitive array type to SoftCoding.
    Declare it directly after the member.
    NOTE: It's cleaner to convert the member to AZStd::array as
    this avoid having to use this special case while preserving semantics.
    Usage:
        ColorB m_colors[20];
        SOFT_ARRAY(m_colors);
*/
    #define SOFT_ARRAY(arrayMember) _SOFT_MEMBER_VISITOR(arrayMember, __COUNTER__)

// Internal: Executes given lambda in an SEH try block.
template <typename TLambda>
void SoftCodeTry(TLambda& lambda)
{
    __try
    {
        lambda();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

// Internal: Used by TypeRegistry. Use SOFTCODE_RETRY() for user code.
    #define SOFTCODE_TRY(exp) SoftCodeTry([&](){ exp; })
    #define SOFTCODE_TRY_BLOCK SoftCodeTry([&]() {
    #define SOFTCODE_TRY_END });

/*
    Internal: Attempt a given lambda functor. In the even on an exception execution will pause
    to allow the user to provide a replacement implementation for the failing instance.
    Usage: See SOFTCODE_RETRY & SOFTCODE_RETRY_BLOCK below.
*/
template <typename TPtr, typename TLambda>
void SoftCodeRetry(TPtr& pointer, TLambda& lambda)
{
    bool complete = false;
    while (pointer && !complete)
    {
        __try
        {
            lambda();
            complete = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (gEnv->pSoftCodeMgr)
            {
                pointer = reinterpret_cast<TPtr>(gEnv->pSoftCodeMgr->WaitForUpdate(pointer));
            }
        }
    }
    ;
}

/*
    Attempts to call an expression based the given pointer.
    If a Structured Exception is raised the thread will wait for a replacement of
    the pointer to be provided via the SoftCodeMgr. This will repeat until no exception is raised.

    Usage: SOFTCODE_RETRY(pThing, (pThing->Update(frameTime)));     // Same as pThing->Update(frameTime);
*/
    #define SOFTCODE_RETRY(pointer, exp) SoftCodeRetry(pointer, [&](){ exp; })

/*
    Attempts to call an expression based the given pointer.
    If a Structured Exception is raised the thread will wait for a replacement of
    the pointer to be provided via the SoftCodeMgr. This will repeat until no exception is raised.

    Usage:
        SOFTCODE_RETRY_BLOCK(pThing)
        {
            pSomething = pThing->GetSomething();
        }
        SOFTCODE_RETRY_END
*/
    #define SOFTCODE_RETRY_BLOCK(pointer) SoftCodeRetry(pointer, [&]() {
    #define SOFTCODE_RETRY_END });

#else   // !SOFTCODE_ENABLED ...

// IMPORTANT: Docs for these macros are found above.

    #define DECLARE_TYPELIB(IName) \
    typedef CTypeLibrary<IName> TLibrary

    #define DECLARE_TYPE(TName, TSuperType) \
private:                                    \
    friend class TypeRegistrar<TName>;

    #define _EXPORT_TYPE_LIB(Interface)

    #define SOFT(member) member
    #define SOFT_ARRAY(arrayMember)

    #define SOFTCODE_TRY(exp) (exp)
    #define SOFTCODE_TRY_BLOCK {
    #define SOFTCODE_TRY_END };

    #define SOFTCODE_RETRY(pointer, exp) (exp)
    #define SOFTCODE_RETRY_BLOCK(pointer) {
    #define SOFTCODE_RETRY_END };

#endif  // !SOFTCODE_ENABLED

/*
    Implements registration for a type to a TypeLibrary.
    Usage:
        // MyThing.cpp
        DECLARE_TYPE(MyThing);
*/
#define IMPLEMENT_TYPE(TName) \
    static TypeRegistrar<TName> s_register##TName(#TName)

/*
    Provides the singleton for the TypeLibrary implementation.
    Also exports the accessors function for SoftCode builds.
    Usage:
    // ThingLibrary.cpp
    IMPLEMENT_TYPELIB(IThing, "Things")
*/
#define IMPLEMENT_TYPELIB(Interface, Name)                       \
    _EXPORT_TYPE_LIB(Interface)                                  \
    template <>                                                  \
    CTypeLibrary<Interface>* CTypeLibrary<Interface>::Instance() \
    {                                                            \
        static CTypeLibrary<Interface> s_instance(Name);         \
        return &s_instance;                                      \
    }

/*
    Internal: Used to register a type with a TypeLibrary.
    Also provides instance construction (factory) access.
    For SC builds it also provides copying and instance tracking.
*/
template <typename T>
class TypeRegistrar
    : public ITypeRegistrar
{
public:
    TypeRegistrar(const char* name)
        : m_name(name)
    {
        typedef typename T::TLibrary TLib;
        TLib::Instance()->RegisterType(this);
    }

    virtual const char* GetName() const { return m_name; }

    virtual void* CreateInstance()
    {
        T* pInstance = NULL;

        SOFTCODE_TRY_BLOCK
        {
            pInstance = ConstructInstance();
        }
        SOFTCODE_TRY_END

        if (pInstance)
        {
            RegisterInstance(pInstance);
        }

        return pInstance;
    }

#ifdef SOFTCODE_ENABLED
    virtual size_t InstanceCount() const
    {
        return m_instances.size();
    }

    virtual void RemoveInstance(InstanceTracker* pTracker)
    {
        T* pInstance = GetInstanceFromTracker(pTracker);
        TInstanceVec::iterator iter(std::find(m_instances.begin(), m_instances.end(), pInstance));
        std::swap(*iter, m_instances.back());
        m_instances.pop_back();
    }

    virtual bool ExchangeInstances(IExchanger& exchanger)
    {
        if (exchanger.IsLoading())
        {
            const size_t instanceCount = exchanger.InstanceCount();

            // Ensure we have the correct number of instances
            if (m_instances.size() != instanceCount)
            {
                // TODO: Destroy any existing instances
                for (size_t i = 0; i < instanceCount; ++i)
                {
                    if (!CreateInstance())
                    {
                        return false;
                    }
                }
            }
        }

        bool complete = false;
        SOFTCODE_TRY_BLOCK
        {
            for (std::vector<T*>::iterator iter(m_instances.begin()); iter != m_instances.end(); ++iter)
            {
                T* pInstance = *iter;
                if (exchanger.BeginInstance(pInstance))
                {
                    // Exchanges the members of pInstance as defined in T
                    // Should also exchange members of parent types
                    pInstance->VisitMembers(exchanger);
                }
            }
            complete = true;
        }
        SOFTCODE_TRY_END

        return complete;
    }

    virtual bool DestroyInstances()
    {
        bool complete = false;
        SOFTCODE_TRY_BLOCK
        {
            while (!m_instances.empty())
            {
                delete m_instances.back();
                // NOTE: No need to pop_back() as already done by the InstanceTracker via RemoveInstance()
            }
            complete = true;
        }
        SOFTCODE_TRY_END

        return complete;
    }


    virtual bool HasInstance(void* pInstance) const
    {
        return std::find(m_instances.begin(), m_instances.end(), pInstance) != m_instances.end();
    }
#endif

private:
    void RegisterInstance(T* pInstance)
    {
#ifdef SOFTCODE_ENABLED
        pInstance->__instanceTracker.SetRegistrar(this);
        m_instances.push_back(pInstance);
#endif
    }

#ifdef SOFTCODE_ENABLED
    // Util: Avoids having to redundantly store the instance address in the tracker
    T* GetInstanceFromTracker(InstanceTracker* pTracker)
    {
        ptrdiff_t trackerOffset = reinterpret_cast<ptrdiff_t>(&((static_cast<T*>(0))->__instanceTracker));
        return reinterpret_cast<T*>(reinterpret_cast<char*>(pTracker) - trackerOffset);
    }
#endif

    // Needed to avoid C2712 due to lack of stack unwind within SEH try blocks
    T* ConstructInstance()
    {
        return new T();
    }

private:
    const char* m_name;                                     // Name of the type

#ifdef SOFTCODE_ENABLED
    typedef std::vector<T*> TInstanceVec;
    TInstanceVec m_instances;               // Tracks the active instances (SC only)
#endif
};

/*
    Provides factory creation support for a set of types that
    derive from a single interface T. Users need to provide a
    specialization of the static CTypeLibrary<T>* Instance() member
    in a cpp file to provide the singleton instance.
*/
template <typename T>
class CTypeLibrary
    #ifdef SOFTCODE_ENABLED
    : public ITypeLibrary
    #endif
{
public:
    CTypeLibrary(const char* name)
        : m_name(name)
#ifdef SOFTCODE_ENABLED
        , m_pOverrideLib()
        , m_overrideActive()
        , m_registered()
#endif
    {
    }

    // Implemented in the export cpp
    static CTypeLibrary<T>* Instance();

    void RegisterType(ITypeRegistrar* pType)
    {
        m_typeMap[pType->GetName()] = pType;
    }

    // The global identifier for this library module
    /*virtual*/ const char* GetName() { return m_name; }

#ifdef SOFTCODE_ENABLED
    virtual void* CreateInstanceVoid(const char* typeName)
    {
        return CreateInstance(typeName);
    }
#endif

    // Generic creation function
    T* CreateInstance(const char* typeName)
    {
#ifdef SOFTCODE_ENABLED
        RegisterWithSoftCode();

        // If override is enabled, use it
        if (m_pOverrideLib)
        {
            return reinterpret_cast<T*>(m_pOverrideLib->CreateInstanceVoid(typeName));
        }
#endif

        TTypeMap::const_iterator typeIter(m_typeMap.find(typeName));
        if (typeIter != m_typeMap.end())
        {
            ITypeRegistrar* pRegistrar = typeIter->second;
            return reinterpret_cast<T*>(pRegistrar->CreateInstance());
        }

        return NULL;
    }

#ifdef SOFTCODE_ENABLED
    // Indicates CreateInstance requests should be forwarded to the specified lib
    virtual void SetOverride(ITypeLibrary* pOverrideLib)
    {
        m_pOverrideLib = pOverrideLib;
    }

    virtual size_t GetTypes(ITypeRegistrar** ppTypes, size_t& count) const
    {
        size_t returnedCount = 0;

        if (ppTypes && count >= m_typeMap.size())
        {
            for (TTypeMap::const_iterator iter(m_typeMap.begin()); iter != m_typeMap.end(); ++iter)
            {
                *ppTypes = iter->second;
                ++ppTypes;
                ++returnedCount;
            }
        }

        count = m_typeMap.size();
        return returnedCount;
    }

    // Inform the Mgr of this Library and allow it to set an override
    inline void RegisterWithSoftCode()
    {
        // Only register built-in types, SC types are handled directly by
        // the SoftCodeMgr, so there's no need to auto-register.
#ifndef SOFTCODE
        if (!m_registered)
        {
            if (ISoftCodeMgr* pSoftCodeMgr = gEnv->pSoftCodeMgr)
            {
                pSoftCodeMgr->RegisterLibrary(this);
            }

            m_registered = true;
        }
#endif
    }
#endif

private:
    typedef AZStd::basic_string<char, AZStd::char_traits<char>, AZ::AZStdAlloc<CryLegacySTLAllocator>> TypeString;
    typedef AZStd::map<TypeString, ITypeRegistrar*, AZStd::less<TypeString>, AZ::AZStdAlloc<CryLegacySTLAllocator>> TTypeMap;
    TTypeMap m_typeMap;

    // The name for this TypeLibrary used during SC registration
    const char* m_name;

#ifdef SOFTCODE_ENABLED
    // Used to patch in a new TypeLib at run-time
    ITypeLibrary* m_pOverrideLib;
    // True when the owning object system enables the override
    bool m_overrideActive;
    // True when registration with SoftCodeMgr has been attempted
    bool m_registered;
#endif
};

#endif // CRYINCLUDE_CRYCOMMON_TYPELIBRARY_H

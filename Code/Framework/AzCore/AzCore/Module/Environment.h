/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_ENVIRONMENT_INCLUDE_H
#define AZCORE_ENVIRONMENT_INCLUDE_H 1

#include <AzCore/std/smart_ptr/sp_convertible.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/spin_mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/has_virtual_destructor.h>
#include <AzCore/std/typetraits/aligned_storage.h>

namespace AZ
{
    namespace Internal
    {
        class EnvironmentInterface;
    }

    typedef Internal::EnvironmentInterface* EnvironmentInstance;

    /// EnvironmentVariable holds a pointer to the actual variable, it should be used as any smart pointer. Event though most of the it will be "static" as it's treated as a global.
    template<class T>
    class EnvironmentVariable;

    /**
     * Environment is a module "global variable" provider. It's used when you want to share globals (which should happen as little as possible)
     * between modules/DLLs.
     *
     * The environment is automatically created on demand. This is why if you want to share the environment between modules you should call \ref
     * AttachEnvironment before anything else that will use the environment. At a high level the environment is basically a hash table with variables<->guid.
     * The environment doesn't own variables they are managed though reference counting.
     *
     * All environment "internal" allocations are directly allocated from OS C heap, so we don't use any allocators as they use the environment themselves.
     *
     * You can create/destroy variables from multiple threads, HOWEVER accessing the data of the environment is NOT. If you want to access your variable
     * across threads you will need to add extra synchronization, which is the same if your global was "static".
     *
     * Using the environment with POD types is straight and simple. On the other hand when your environment variable has virtual methods, things become more
     * complex. We advice that you avoid this as much as possible. When a variable has virtual function, even if we shared environment and memory allocations,
     * the variable will still be "bound" to the module as it's vtable will point inside the module. Which mean that if we unload the module we have to destroy
     * your variable. The system handles this automatically if your class has virtual functions. You can still keep your variables, but if you try to access the data
     * you will trigger an assert, unless you recreated the variable from one of the exiting modules.
     *
     * \note we use class for the Environment (instead of simple global function) so can easily friend it to create variables where the ctor/dtor are protected
     * that way you need to add friend AZ::Environment and that's it.
     */
    namespace Environment
    {
        /**
         * Allocator hook interface. This should not use any of the AZ allocators or interfaces as it will be called to create them.
         */
        class AllocatorInterface
        {
        public:

            virtual void* Allocate(size_t byteSize, size_t alignment) = 0;

            virtual void DeAllocate(void* address) = 0;
        };

        /**
         * Creates an environmental variable, if the variable already exists just returns the exiting variable.
         * If not it will create one and return the new variable.
         * \param guid - unique persistent id for the variable. It will be matched between modules/different and compilations.
         * The returned variable will be release and it's memory freed when the last reference to it is gone.
         */
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariable(u32 guid, Args&&... args);

        /**
         * Same as \ref CreateVariable except it uses a unique string to identify the variable, this string will be converted to Crc32 and used a guid.
         */
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariable(const char* uniqueName, Args&&... args);


        /**
         * Advanced \ref CreateVariable where you have extra parameters to control the creation and life cycle of the variable.
         * \param guid - same as \ref CreateVariable guid
         * \param isConstruct - if we should actually "create" the variable. If false we will just register the variable holder, but EnvironmentVariable<T>::IsConstructed will return false.
         * you will need to manually create it later with EnvironmentVariable<T>::Construct().
         * \param isTransferOwnership - if we can transfer ownership of the variable across modules.
         * For example Module_1 created the variable and Module_2 is using it. If is isTransferOwnership is true if you unload Module_1, Module_2 will get the ownership.
         * If isTransferOwnership is false, it will destroy the variable and any attempt from Module_2 of using will trigger and error (until somebody else creates it again).
         */
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariableEx(u32 guid, bool isConstruct, bool isTransferOwnership, Args&&... args);

        /// Same as \ref CreateVariableEx and \ref CreateVariable with advanced control parameters.
        template <typename T, class... Args>
        EnvironmentVariable<T>  CreateVariableEx(const char* uniqueName, bool isConstruct, bool isTransferOwnership, Args&&... args);

        /**
         * Check if a variable exist if so a valid variable is returned, otherwise the resulting variable pointer will be null.
         */
        template <typename T>
        EnvironmentVariable<T>  FindVariable(u32 guid);


        /// Same as \ref FindVariable but using a unique string as input. Please check \ref CreateVariable for more information about hot strings are used internally.
        template <typename T>
        EnvironmentVariable<T>  FindVariable(const char* uniqueName);

        /// Returns the environment so you can share it with \ref AttachEnvironment
        EnvironmentInstance GetInstance();

        /// Returns module id (non persistent)
        void* GetModuleId();

        /**
         * Create Environment with customer allocator interface. You don't have to call create or destroy as they will
         * created on demand, but is such case the module allocator will used. For example on Windows if you link the CRT
         * two environments will end up on different heaps.
         * \returns true if Create was successful, false if environment is already created/attached.
         */
        bool Create(AllocatorInterface* allocator);

        /**
         * Explicit Destroy, you don't have to call it unless you want to control order. It will be called when the module is unloaded.
         * Of course no order is guaranteed.
         */
        void Destroy();

        /**
         * Attaches the current module environment from sourceEnvironment.
         * note: this is not a copy it will actually reference the source environment, so any variables
         * you add remove will be visible to all shared modules
         * \param useAsFallback if set to true a new environment will be created and only failures to GetVariable
         * which check the shared environment. This way you can change the environment.
         */
        void Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback = false);

        /// Detaches the active environment (if one is attached)
        void Detach();

        /// Returns true if an environment is attached to this module
        bool IsReady();
    };

    namespace Internal
    {
        struct EnvironmentVariableResult
        {
            enum States
            {
                Added,
                Removed,
                Found,
                NotFound,
                OutOfMemory,
            };

            States m_state;
            void*  m_variable;
        };

        /**
        * Environment interface class
        */
        class EnvironmentInterface
        {
        public:
            virtual ~EnvironmentInterface() {}

            virtual AZStd::recursive_mutex& GetLock() = 0;

            virtual void AttachFallback(EnvironmentInstance sourceEnvironment) = 0;

            virtual void DetachFallback() = 0;

            virtual EnvironmentInterface* GetFallback() = 0;

            virtual void* FindVariable(u32 guid) = 0;

            virtual EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, AZStd::recursive_mutex** addedVariableLock) = 0;

            virtual EnvironmentVariableResult RemoveVariable(u32 guid) = 0;

            virtual EnvironmentVariableResult GetVariable(u32 guid) = 0;

            virtual void AddRef() = 0;

            virtual void ReleaseRef() = 0;

            virtual Environment::AllocatorInterface* GetAllocator() = 0;

            virtual void DeleteThis() = 0;

            static EnvironmentInterface*  s_environment;
        };

        // Don't use any virtual methods to we make sure we can reuse variables across modules
        class EnvironmentVariableHolderBase
        {
            friend class EnvironmentImpl;
        protected:
            enum class DestroyTarget {
                Member,
                Self
            };
        public:
            EnvironmentVariableHolderBase(u32 guid, AZ::Internal::EnvironmentInterface* environmentOwner, bool canOwnershipTransfer, Environment::AllocatorInterface* allocator)
                : m_environmentOwner(environmentOwner)
                , m_moduleOwner(nullptr)
                , m_canTransferOwnership(canOwnershipTransfer)
                , m_isConstructed(false)
                , m_guid(guid)
                , m_allocator(allocator)
                , m_useCount(0)
            {
            }

            bool IsConstructed() const
            {
                return m_isConstructed;
            }

            bool IsOwner() const
            {
                return m_moduleOwner == Environment::GetModuleId();
            }

            u32 GetId() const
            {
                return m_guid;
            }
        protected:
            using DestructFunc = void (*)(EnvironmentVariableHolderBase *, DestroyTarget);
            // Assumes the m_mutex is already locked.
            // On return m_mutex is in an unlocked state.
            void UnregisterAndDestroy(DestructFunc destruct, bool moduleRelease);

            AZ::Internal::EnvironmentInterface* m_environmentOwner; ///< Used to know which environment we should use to free the variable if we can't transfer ownership
            void* m_moduleOwner; ///< Used when the variable can't transfered across module and we need to destruct the variable when the module is going away
            bool m_canTransferOwnership; ///< True if variable can be allocated in one module and freed in other. Usually true for POD types when they share allocator.
            bool m_isConstructed; ///< When we can't transfer the ownership, and the owning module is destroyed we have to "destroy" the variable.
            u32 m_guid;
            Environment::AllocatorInterface* m_allocator;
            int m_useCount;
            AZStd::spin_mutex m_mutex;
        };

        template<typename T>
        class EnvironmentVariableHolder
            : public EnvironmentVariableHolderBase
        {
            template<class... Args>
            void ConstructImpl(Args&&... args)
            {
                // Use std::launder to ensure that the compiler treats the T* reinterpret_cast as a new object
            #if __cpp_lib_launder
                AZStd::construct_at(std::launder(reinterpret_cast<T*>(&m_value)), AZStd::forward<Args>(args)...);
            #else
                AZStd::construct_at(reinterpret_cast<T*>(&m_value), AZStd::forward<Args>(args)...);
            #endif
            }
            static void DestructDispatchNoLock(EnvironmentVariableHolderBase *base, DestroyTarget selfDestruct)
            {
                auto *self = reinterpret_cast<EnvironmentVariableHolder *>(base);
                if (selfDestruct == DestroyTarget::Self)
                {
                    self->~EnvironmentVariableHolder();
                    return;
                }

                AZ_Assert(self->m_isConstructed, "Variable is not constructed. Please check your logic and guard if needed!");
                self->m_isConstructed = false;
                self->m_moduleOwner = nullptr;
                // Use std::launder to ensure that the compiler treats the T* reinterpret_cast as a new object
            #if __cpp_lib_launder
                AZStd::destroy_at(std::launder(reinterpret_cast<T*>(&self->m_value)));
            #else
                AZStd::destroy_at(reinterpret_cast<T*>(&self->m_value));
            #endif
            }
        public:
            EnvironmentVariableHolder(u32 guid, bool isOwnershipTransfer, Environment::AllocatorInterface* allocator)
                : EnvironmentVariableHolderBase(guid, Environment::GetInstance(), isOwnershipTransfer, allocator)
            {
            }

            ~EnvironmentVariableHolder()
            {
                AZ_Assert(!m_isConstructed, "To get the destructor we should have already destructed the variable!");
            }
            void AddRef()
            {
                AZStd::lock_guard<AZStd::spin_mutex> lock(m_mutex);
                ++m_useCount;
                ++s_moduleUseCount;
            }

            void Release()
            {
                m_mutex.lock();
                const bool moduleRelease = (--s_moduleUseCount == 0);
                UnregisterAndDestroy(DestructDispatchNoLock, moduleRelease);
            }

            template <class... Args>
            void Construct(Args&&... args)
            {
                AZStd::lock_guard<AZStd::spin_mutex> lock(m_mutex);
                if (!m_isConstructed)
                {
                    ConstructImpl(AZStd::forward<Args>(args)...);
                    m_isConstructed = true;
                    m_moduleOwner = Environment::GetModuleId();
                }
            }

            void Destruct()
            {
                AZStd::lock_guard<AZStd::spin_mutex> lock(m_mutex);
                DestructDispatchNoLock(this, DestroyTarget::Member);
            }

            // variable storage
            AZStd::aligned_storage_for_t<T> m_value;
            static int s_moduleUseCount;
        };

        template<typename T>
        int EnvironmentVariableHolder<T>::s_moduleUseCount = 0;

        /**
        * Returns a pointer to the newly allocated variable, if the variable doesn't exists it' will be created.
        * If you provide addedVariableLock you will receive lock if a variable has been created, so you can safely construct the object and then
        * release the lock.
        */
        EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, AZStd::recursive_mutex** addedVariableLock = nullptr);

        /// Returns the value of the variable if found, otherwise nullptr.
        EnvironmentVariableResult GetVariable(u32 guid);

        /// Returns the allocator used by the current environment.
        Environment::AllocatorInterface* GetAllocator();

        /// Converts a string name to an ID (using Crc32 function)
        u32  EnvironmentVariableNameToId(const char* uniqueName);
    } // namespace Internal

    /**
     * EnvironmentVariable implementation, T should be default constructible/destructible.
     * Keep in mind that if T uses virtual function (virtual destructor), it will use the vtable for the current module
     * unloading the module with a vtable will destroy the variable (unless specified with CreateVariableEx). You will either need to control the order
     * (aka Call Environmnet::Create from the module that will alive before the variable is destroyed) or provide intramodule allocator
     * \ref Environment::Create and make sure you don't use virtual function (of course implementations must match too).
     * In general we advise to use the minimal amount of environment variables and use pointer types so you can manage the life cycle yourself,
     * we will accept value types and do the best to make it work for you.
     * When it comes to threading we make sure the EnvironmentVariable objects can be created, destroyed and assigned from multiple threads, however
     * we don't make the code safe (for performance) when you access the variable. If your variable can be access from multiple threads while it's being
     * modified you will need to make sure you synchronize the access to it.
     */
    template<class T>
    class EnvironmentVariable
    {
    public:
        typedef typename AZ::Internal::EnvironmentVariableHolder<T> HolderType;

        EnvironmentVariable()
            : m_data(nullptr)
        {
        }

        EnvironmentVariable(HolderType* holder)
            : m_data(holder)
        {
            if (m_data != nullptr)
            {
                m_data->AddRef();
            }
        }

        template<class U>
        EnvironmentVariable(EnvironmentVariable<U> const& rhs, typename AZStd::Internal::sp_enable_if_convertible<U, T>::type = AZStd::Internal::sp_empty())
            : m_data(rhs.m_data)
        {
            if (m_data != nullptr)
            {
                m_data->AddRef();
            }
        }
        EnvironmentVariable(EnvironmentVariable const& rhs)
            : m_data(rhs.m_data)
        {
            if (m_data != nullptr)
            {
                m_data->AddRef();
            }
        }
        ~EnvironmentVariable()
        {
            if (m_data != nullptr)
            {
                m_data->Release();
            }
        }
        template<class U>
        EnvironmentVariable& operator=(EnvironmentVariable<U> const& rhs)
        {
            EnvironmentVariable(rhs).Swap(*this);
            return *this;
        }

        EnvironmentVariable(EnvironmentVariable&& rhs)
            : m_data(rhs.m_data)
        {
            rhs.m_data = nullptr;
        }
        EnvironmentVariable& operator=(EnvironmentVariable&& rhs)
        {
            EnvironmentVariable(static_cast< EnvironmentVariable && >(rhs)).Swap(*this);
            return *this;
        }

        EnvironmentVariable& operator=(EnvironmentVariable const& rhs)
        {
            EnvironmentVariable(rhs).Swap(*this);
            return *this;
        }

        void Reset()
        {
            EnvironmentVariable().Swap(*this);
        }

        T& operator*() const
        {
            AZ_Assert(IsValid(), "You can't dereference a null pointer");
            AZ_Assert(m_data->IsConstructed(), "You are using an invalid variable, the owner has removed it!");
            return *reinterpret_cast<T*>(&m_data->m_value);
        }

        T* operator->() const
        {
            AZ_Assert(IsValid(), "You can't dereference a null pointer");
            AZ_Assert(m_data->IsConstructed(), "You are using an invalid variable, the owner has removed it!");
            return reinterpret_cast<T*>(&m_data->m_value);
        }

        T& Get() const
        {
            AZ_Assert(IsValid(), "You can't dereference a null pointer");
            AZ_Assert(m_data->IsConstructed(), "You are using an invalid variable, the owner has removed it!");
            return *reinterpret_cast<T*>(&m_data->m_value);
        }

        void Set(const T& value)
        {
            Get() = value;
        }

        void Set(T&& value)
        {
            Get() = AZStd::move(value);
        }

        explicit operator bool() const
        {
            return IsValid();
        }
        bool operator! () const { return !IsValid(); }

        void Swap(EnvironmentVariable& rhs)
        {
            HolderType* tmp = m_data;
            m_data = rhs.m_data;
            rhs.m_data = tmp;
        }

        bool IsValid() const
        {
            return m_data;
        }

        bool IsOwner() const
        {
            return m_data && m_data->IsOwner();
        }

        bool IsConstructed() const
        {
            return m_data && m_data->IsConstructed();
        }

        void Construct()
        {
            AZ_Assert(IsValid(), "You can't dereference a null pointer");
            m_data->Construct();
        }

    protected:
        HolderType* m_data;
    };

    template <class T, class U>
    inline bool operator==(EnvironmentVariable<T> const& a, EnvironmentVariable<U> const& b)
    {
        return a.m_data == b.m_data;
    }

    template <class T, class U>
    inline bool operator!=(EnvironmentVariable<T> const& a, EnvironmentVariable<U> const& b)
    {
        return a.m_data != b.m_data;
    }

    template <class T, class U>
    inline bool operator==(EnvironmentVariable<T> const& a, U* b)
    {
        return a.m_data == b;
    }

    template <class T, class U>
    inline bool operator!=(EnvironmentVariable<T> const& a, U* b)
    {
        return a.m_data != b;
    }

    template <class T, class U>
    inline bool operator==(T* a, EnvironmentVariable<U> const& b)
    {
        return a == b.m_data;
    }

    template <class T, class U>
    inline bool operator!=(T* a, EnvironmentVariable<U> const& b)
    {
        return a != b.m_data;
    }

    template <class T>
    bool operator==(EnvironmentVariable<T> const& a, std::nullptr_t)
    {
        return !a.IsValid();
    }

    template <class T>
    bool operator!=(EnvironmentVariable<T> const& a, std::nullptr_t)
    {
        return a.IsValid();
    }

    template <class T>
    bool operator==(std::nullptr_t, EnvironmentVariable<T> const& a)
    {
        return !a.IsValid();
    }

    template <class T>
    bool operator!=(std::nullptr_t, EnvironmentVariable<T> const& a)
    {
        return a.IsValid();
    }

    template <class T>
    inline bool operator<(EnvironmentVariable<T> const& a, EnvironmentVariable<T> const& b)
    {
        return a.m_data < b.m_data;
    }

    template <typename T, class... Args>
    EnvironmentVariable<T>  Environment::CreateVariable(u32 guid, Args&&... args)
    {
        // If T has virtual functions, we can't transfer ownership even if we share
        // allocators, because the vtable will point to memory inside the module that created the variable.
        bool isTransferOwnership = !AZStd::has_virtual_destructor<T>::value;
        return CreateVariableEx<T>(guid, true, isTransferOwnership, AZStd::forward<Args>(args)...);
    }

    template <typename T, class... Args>
    EnvironmentVariable<T> Environment::CreateVariable(const char* uniqueName, Args&&... args)
    {
        return CreateVariable<T>(AZ::Internal::EnvironmentVariableNameToId(uniqueName), AZStd::forward<Args>(args)...);
    }

    template <typename T, class... Args>
    EnvironmentVariable<T>  Environment::CreateVariableEx(u32 guid, bool isConstruct, bool isTransferOwnership, Args&&... args)
    {
        typedef typename EnvironmentVariable<T>::HolderType HolderType;
        HolderType* variable = nullptr;
        AZStd::recursive_mutex* addLock = nullptr;
        AZ::Internal::EnvironmentVariableResult result = AZ::Internal::AddAndAllocateVariable(guid, sizeof(HolderType), AZStd::alignment_of<HolderType>::value, &addLock);
        if (result.m_state == AZ::Internal::EnvironmentVariableResult::Added)
        {
            variable = new(result.m_variable)HolderType(guid, isTransferOwnership, AZ::Internal::GetAllocator());
        }
        else if (result.m_state == AZ::Internal::EnvironmentVariableResult::Found)
        {
            variable = reinterpret_cast<HolderType*>(result.m_variable);
        }

        if (isConstruct && variable)
        {
            variable->Construct(AZStd::forward<Args>(args)...);
        }

        if (addLock)
        {
            addLock->unlock();
        }

        return variable;
    }

    /// Same as \ref CreateVariableEx and \ref CreateVariable with advanced control parameters.
    template <typename T, class... Args>
    EnvironmentVariable<T>   Environment::CreateVariableEx(const char* uniqueName, bool isConstruct, bool isTransferOwnership, Args&&... args)
    {
        return CreateVariableEx<T>(Internal::EnvironmentVariableNameToId(uniqueName), isConstruct, isTransferOwnership, AZStd::forward<Args>(args)...);
    }

    template <typename T>
    EnvironmentVariable<T> Environment::FindVariable(u32 guid)
    {
        typedef typename EnvironmentVariable<T>::HolderType HolderType;
        AZ::Internal::EnvironmentVariableResult result = AZ::Internal::GetVariable(guid);
        if (result.m_state == AZ::Internal::EnvironmentVariableResult::Found)
        {
            return reinterpret_cast<HolderType*>(result.m_variable);
        }
        return nullptr;
    }

    template <typename T>
    EnvironmentVariable<T>  Environment::FindVariable(const char* uniqueName)
    {
        return FindVariable<T>(AZ::Internal::EnvironmentVariableNameToId(uniqueName));
    }

    namespace Internal
    {
        inline void AttachGlobalEnvironment(void* globalEnv)
        {
            AZ_Assert(!AZ::Environment::IsReady(), "An environment is already created in this module!");
            AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(globalEnv));
        }
    }

} // namespace AZ

#ifdef AZ_MONOLITHIC_BUILD
#define AZ_DECLARE_MODULE_INITIALIZATION
#else

/// Default implementations of functions invoked upon loading a dynamic module.
/// For dynamic modules which define a \ref AZ::Module class, use \ref AZ_DECLARE_MODULE_CLASS instead.
///
/// For more details see:
/// \ref AZ::InitializeDynamicModuleFunction, \ref AZ::UninitializeDynamicModuleFunction
#define AZ_DECLARE_MODULE_INITIALIZATION \
    extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env) \
    { \
        AZ::Internal::AttachGlobalEnvironment(env); \
    } \
    extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule() { AZ::Environment::Detach(); }

#endif // AZ_MONOLITHIC_BUILD

#endif // AZCORE_PLATFORM_INCLUDE_H
#pragma once



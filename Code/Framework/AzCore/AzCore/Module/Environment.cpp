/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Environment.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    namespace Internal
    {
        /**
        * AZStd allocator wrapper for the global variables. Don't expose to external code, this allocation are NOT
        * tracked, etc. These are only for global environment variables.
        */
        class OSStdAllocator
        {
        public:
            typedef void*               pointer_type;
            typedef AZStd::size_t       size_type;
            typedef AZStd::ptrdiff_t    difference_type;
            typedef AZStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

            OSStdAllocator(Environment::AllocatorInterface* allocator)
                : m_name("GlobalEnvironmentAllocator")
                , m_allocator(allocator)
            {
            }

            OSStdAllocator(const OSStdAllocator& rhs)
                : m_name(rhs.m_name)
                , m_allocator(rhs.m_allocator)
            {
            }

            pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
            {
                (void)flags;
                return m_allocator->Allocate(byteSize, alignment);
            }
            size_type resize(pointer_type ptr, size_type newSize)
            {
                (void)ptr;
                (void)newSize;
                return 0; // no resize
            }
            void deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
            {
                (void)byteSize;
                (void)alignment;
                m_allocator->DeAllocate(ptr);
            }

            const char* get_name() const            { return m_name; }
            void        set_name(const char* name)  { m_name = name; }
            constexpr size_type max_size() const    { return AZ_CORE_MAX_ALLOCATOR_SIZE; }
            size_type   get_allocated_size() const  { return 0; }

            bool is_lock_free()                     { return false; }
            bool is_stale_read_allowed()            { return false; }
            bool is_delayed_recycling()             { return false; }

        private:
            const char* m_name;
            Environment::AllocatorInterface* m_allocator;
        };

        bool operator==(const OSStdAllocator& a, const OSStdAllocator& b) { (void)a; (void)b; return true; }
        bool operator!=(const OSStdAllocator& a, const OSStdAllocator& b) { (void)a; (void)b; return false; }

        void EnvironmentVariableHolderBase::UnregisterAndDestroy(DestructFunc destruct, bool moduleRelease)
        {
            const bool releaseByUseCount = (--m_useCount == 0);
            // We take over the lock, and release it before potentially destroying/freeing ourselves
            {
                AZStd::scoped_lock envLockHolder(AZStd::adopt_lock, m_mutex);
                const bool releaseByModule = (moduleRelease && !m_canTransferOwnership && m_moduleOwner == AZ::Environment::GetModuleId());

                if (!releaseByModule && !releaseByUseCount)
                {
                    return;
                }
                // if the environment that created us is gone the owner can be null
                // which means (assuming intermodule allocator) that the variable is still alive
                // but can't be found as it's not part of any environment.
                if (m_environmentOwner)
                {
                    m_environmentOwner->RemoveVariable(m_guid);
                    m_environmentOwner = nullptr;
                }
                if (m_isConstructed)
                {
                    destruct(this, DestroyTarget::Member); // destruct the value
                }
            }
            // m_mutex is no longer held here, envLockHolder has released it above.
            if (releaseByUseCount)
            {
                // m_mutex is unlocked before this is deleted
                Environment::AllocatorInterface* allocator = m_allocator;
                // Call child class dtor and clear the memory
                destruct(this, DestroyTarget::Self);
                allocator->DeAllocate(this);
            }
        }

        // instance of the environment
        EnvironmentInterface* EnvironmentInterface::s_environment = nullptr;

        /**
         *
         */
        class EnvironmentImpl
            : public EnvironmentInterface
        {
        public:
            typedef AZStd::unordered_map<u32, void*, AZStd::hash<u32>, AZStd::equal_to<u32>, OSStdAllocator> MapType;

            static EnvironmentInterface* Get();
            static void Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback);
            static void Detach();

            EnvironmentImpl(Environment::AllocatorInterface* allocator)
                : m_variableMap(MapType::hasher(), MapType::key_eq(), OSStdAllocator(allocator))
                , m_numAttached(0)
                , m_fallback(nullptr)
                , m_allocator(allocator)
            {}

            ~EnvironmentImpl() override
            {
                if (m_numAttached > 0)
                {
                    // Is is not reasonable to throw an assert here, as the allocators are likely to have already been torn down
                    // and we will just cause a crash that does not help the user or developer.
                    fprintf(stderr, "We should not delete an environment while there are %d modules attached! Unload all DLLs first!", m_numAttached);
                }

#ifdef AZ_ENVIRONMENT_VALIDATE_ON_EXIT
                AZ_Assert(m_numAttached == 0, "We should not delete an environment while there are %d modules attached! Unload all DLLs first!", m_numAttached);
#endif

                for (const auto &variableIt : m_variableMap)
                {
                    EnvironmentVariableHolderBase* holder = reinterpret_cast<EnvironmentVariableHolderBase*>(variableIt.second);
                    if (holder)
                    {
                        AZ_Assert(static_cast<EnvironmentImpl*>(holder->m_environmentOwner) == this, "We should be the owner of all variables in the map");
                        // since we are going away (no more global look up) remove ourselves, but leave the variable allocated so we can have static variables
                        holder->m_environmentOwner = nullptr;
                    }
                }
            }

            AZStd::recursive_mutex& GetLock() override
            {
                return m_globalLock;
            }

            void AttachFallback(EnvironmentInstance sourceEnvironment) override
            {
                m_fallback = sourceEnvironment;
                if (m_fallback)
                {
                    m_fallback->AddRef();
                }
            }

            void DetachFallback() override
            {
                if (m_fallback)
                {
                    m_fallback->ReleaseRef();
                    m_fallback = nullptr;
                }
            }

            EnvironmentInterface* GetFallback() override
            {
                return m_fallback;
            }

            void* FindVariable(u32 guid) override
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end())
                {
                    return variableIt->second;
                }
                else
                {
                    return nullptr;
                }
            }

            EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, AZStd::recursive_mutex** addedVariableLock) override
            {
                EnvironmentVariableResult result;
                m_globalLock.lock();
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end() && variableIt->second != nullptr)
                {
                    result.m_state = EnvironmentVariableResult::Found;
                    result.m_variable = variableIt->second;
                    m_globalLock.unlock();
                    return result;
                }

                if (variableIt == m_variableMap.end()) // if we did not find it or it's not an override look at the fallback
                {
                    if (m_fallback)
                    {
                        AZStd::lock_guard<AZStd::recursive_mutex> fallbackLack(m_fallback->GetLock());
                        void* variable = m_fallback->FindVariable(guid);
                        if (variable)
                        {
                            result.m_state = EnvironmentVariableResult::Found;
                            result.m_variable = variable;
                            m_globalLock.unlock();
                            return result;
                        }
                    }
                }

                auto variableItBool = m_variableMap.insert_key(guid);
                variableItBool.first->second = m_allocator->Allocate(byteSize, alignment);
                if (variableItBool.first->second)
                {
                    result.m_state = EnvironmentVariableResult::Added;
                    result.m_variable = variableItBool.first->second;
                    if (addedVariableLock)
                    {
                        *addedVariableLock = &m_globalLock; // let the user release it
                    }
                    else
                    {
                        m_globalLock.unlock();
                    }
                }
                else
                {
                    result.m_state = EnvironmentVariableResult::OutOfMemory;
                    result.m_variable = nullptr;
                    m_variableMap.erase(variableItBool.first);
                    m_globalLock.unlock();
                }

                return result;
            }

            EnvironmentVariableResult RemoveVariable(u32 guid) override
            {
                // No need to lock, as this function is called already with us owning the lock
                EnvironmentVariableResult result;
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end())
                {
                    result.m_state = EnvironmentVariableResult::Removed;
                    result.m_variable = variableIt->second;
                    m_variableMap.erase(variableIt);
                }
                else
                {
                    result.m_state = EnvironmentVariableResult::NotFound;
                    result.m_variable = nullptr;
                }
                return result;
            }

            EnvironmentVariableResult GetVariable(u32 guid) override
            {
                EnvironmentVariableResult result;
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end())
                {
                    if (m_fallback && variableIt->second == nullptr)
                    {
                        // this is an override to ignore the find
                        result.m_state = EnvironmentVariableResult::NotFound;
                        result.m_variable = nullptr;
                    }
                    else
                    {
                        result.m_state = EnvironmentVariableResult::Found;
                        result.m_variable = variableIt->second;
                    }
                }
                else
                {
                    if (m_fallback)
                    {
                        return m_fallback->GetVariable(guid);
                    }
                    else
                    {
                        result.m_state = EnvironmentVariableResult::NotFound;
                        result.m_variable = nullptr;
                    }
                }

                return result;
            }

            void AddRef() override
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                m_numAttached++;
            }

            void ReleaseRef() override
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                m_numAttached--;
            }

            void DeleteThis() override
            {
                Environment::AllocatorInterface* allocator = m_allocator;
                this->~EnvironmentImpl();
                allocator->DeAllocate(this);
            }

            Environment::AllocatorInterface* GetAllocator() override
            {
                return m_allocator;
            }

            MapType m_variableMap;

            AZStd::recursive_mutex                m_globalLock;  ///< Mutex that controls access to all environment resources.

            unsigned int                m_numAttached; ///< used for "correctness" checks.
            EnvironmentInterface*       m_fallback;    ///< If set we will use the fallback environment for GetVariable operations.
            Environment::AllocatorInterface* m_allocator;
        };


        /**
        * Destructor will be called when we unload the module.
        */
        class CleanUp
        {
        public:
            CleanUp()
                : m_isOwner(false)
                , m_isAttached(false)
            {
            }
            ~CleanUp()
            {
                if (EnvironmentImpl::s_environment)
                {
                    if (m_isAttached)
                    {
                        EnvironmentImpl::Detach();
                    }
                    if (m_isOwner)
                    {
                        EnvironmentImpl::s_environment->DeleteThis();
                        EnvironmentImpl::s_environment = nullptr;
                    }
                }
            }

            bool m_isOwner;        ///< This is not really needed just to make sure the compiler doesn't optimize the code out.
            bool m_isAttached;     ///< True if this environment is attached in anyway.
        };

        static CleanUp      g_environmentCleanUp;

        EnvironmentInterface* EnvironmentImpl::Get()
        {
            if (!s_environment)
            {
                // create with default allocator OS allocator. They user can provide custom, but he needs
                // to call Create before we needed/fist use.
                Environment::Create(nullptr);
            }

            return s_environment;
        }

        void EnvironmentImpl::Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback)
        {
            if (!sourceEnvironment)
            {
                return;
            }

            Detach();

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(sourceEnvironment->GetLock());
                if (useAsGetFallback)
                {
                    Get(); // create new environment
                    s_environment->AttachFallback(sourceEnvironment);
                }
                else
                {
                    s_environment = sourceEnvironment;
                    s_environment->AttachFallback(nullptr);
                    s_environment->AddRef();
                }
            }

            g_environmentCleanUp.m_isAttached = true;
        }

        void EnvironmentImpl::Detach()
        {
            if (s_environment && g_environmentCleanUp.m_isAttached)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(s_environment->GetLock());
                if (g_environmentCleanUp.m_isOwner)
                {
                    if (s_environment->GetFallback())
                    {
                        AZStd::lock_guard<AZStd::recursive_mutex> fallbackLock(s_environment->GetFallback()->GetLock());
                        s_environment->DetachFallback();
                    }
                }
                else
                {
                    s_environment->ReleaseRef();
                    s_environment = nullptr;
                }

                g_environmentCleanUp.m_isAttached = false;
            }
        }

        EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, AZStd::recursive_mutex** addedVariableLock)
        {
            return EnvironmentImpl::Get()->AddAndAllocateVariable(guid, byteSize, alignment, addedVariableLock);
        }

        EnvironmentVariableResult GetVariable(u32 guid)
        {
            return EnvironmentImpl::Get()->GetVariable(guid);
        }

        Environment::AllocatorInterface* GetAllocator()
        {
            return EnvironmentImpl::Get()->GetAllocator();
        }

        u32 EnvironmentVariableNameToId(const char* uniqueName)
        {
            return Crc32(uniqueName);
        }
    } // namespace Internal

    namespace Environment
    {
        bool IsReady()
        {
            return Internal::EnvironmentInterface::s_environment != nullptr;
        }

        EnvironmentInstance GetInstance()
        {
            return Internal::EnvironmentImpl::Get();
        }

        void* GetModuleId()
        {
            return &Internal::g_environmentCleanUp;
        }

        class ModuleAllocator
            : public AllocatorInterface
        {
        public:
            void* Allocate(size_t byteSize, size_t alignment) override { return AZ_OS_MALLOC(byteSize, alignment); }

            void DeAllocate(void* address) override { AZ_OS_FREE(address); }
        };

        bool Create(AllocatorInterface* allocator)
        {
            if (Internal::EnvironmentImpl::s_environment)
            {
                return false;
            }

            if (!allocator)
            {
                static ModuleAllocator s_moduleAllocator;
                allocator = &s_moduleAllocator;
            }

            Internal::EnvironmentImpl::s_environment = new(allocator->Allocate(sizeof(Internal::EnvironmentImpl), AZStd::alignment_of<Internal::EnvironmentImpl>::value)) Internal::EnvironmentImpl(allocator);
            AZ_Assert(Internal::EnvironmentImpl::s_environment, "We failed to allocate memory from the OS for environment storage %d bytes!", sizeof(Internal::EnvironmentImpl));
            Internal::g_environmentCleanUp.m_isOwner = true;

            return true;
        }

        void Destroy()
        {
            if (!Internal::g_environmentCleanUp.m_isAttached && Internal::g_environmentCleanUp.m_isOwner)
            {
                Internal::g_environmentCleanUp.m_isOwner = false;
                Internal::g_environmentCleanUp.m_isAttached = false;

                Internal::EnvironmentImpl::s_environment->DeleteThis();
                Internal::EnvironmentImpl::s_environment = nullptr;
            }
            else
            {
                Detach();
            }
        }

        void Detach()
        {
            Internal::EnvironmentImpl::Detach();
        }

        void Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback)
        {
            Internal::EnvironmentImpl::Attach(sourceEnvironment, useAsGetFallback);
        }
    } // namespace Environment
} // namespace AZ

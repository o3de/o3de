/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/utils.h>

// InstancePools hold and recycle arbitrary C++ objects and are useful for objects that are expensive to create and destroy.
// They are created by the InstancePoolManager whose lifetime is managed by AzFramework::Application

namespace AZ
{
    class InstancePoolBase
    {
    };

    template<typename T>
    class InstancePool : public InstancePoolBase
    {
    public:
        using ResetInstance = AZStd::function<void(T&)>;

        InstancePool(ResetInstance resetFunctor = [](T&){})
            : m_resetFunction(resetFunctor)
        {
        }

        // returns a recycled instance if present, but creates a new instance if necessary
        T* GetInstance()
        {
            if (!m_instances.empty())
            {
                T* fromList = m_instances.back().release();
                m_instances.pop_back();
                return fromList;
            }
            else
            {
                return new T;
            }
        }

        // calls the pool's reset function on the passed instance, then adds it to the pool
        void RecycleInstance(T* instanceToRecycle)
        {
            if (instanceToRecycle == nullptr)
            {
                return;
            }
            if (m_resetFunction)
            {
                m_resetFunction(*instanceToRecycle);
            }

            m_instances.emplace_back(AZStd::unique_ptr<T>(instanceToRecycle));
        }

    private:
        AZStd::vector<AZStd::unique_ptr<T>> m_instances;
        ResetInstance m_resetFunction;
    };

    class InstancePoolManagerInterface
    {
    public:
        AZ_RTTI(InstancePoolManagerInterface, "{58980615-BC01-470F-8056-D2E2EFDFF27E}");
    };

    class InstancePoolManager : public InstancePoolManagerInterface
    {
    public:
        InstancePoolManager()
        {
            AZ::Interface<InstancePoolManagerInterface>::Register(this);
        }

        virtual ~InstancePoolManager()
        {
            AZ::Interface<InstancePoolManagerInterface>::Unregister(this);
        }

        // creates a pool with the explicit name given, and the reset function specified
        template<class T>
        AZ::Outcome<AZStd::shared_ptr<InstancePool<T>>, AZStd::string> CreatePool(AZ::Name name, typename InstancePool<T>::ResetInstance resetFunc)
        {
            AZStd::shared_ptr<InstancePool<T>> instancePool = AZStd::make_shared<InstancePool<T>>(resetFunc);
            auto insertResult = m_nameToInstancePool.insert({ name, instancePool });
            if (!insertResult.second)
            {
                // Insert failed because an entry already exists. See if the weak pointer at that location is 
                // lockable. If not, remove the weak_ptr and insert a new one
                if (insertResult.first->second.lock())
                {
                    return AZ::Failure(AZStd::string::format("Instance already exist with name %.s", name.GetCStr()));
                }
                else
                {
                    m_nameToInstancePool.erase(insertResult.first);
                    m_nameToInstancePool.insert({ name, instancePool });
                    return AZ::Success(instancePool);
                }
            }
            else
            {
                return AZ::Success(instancePool);
            }
        }

        // same as above, but uses the class's AzTypeInfo name as the pool name. Useful when sharing pools across different editors
        // without having to manually coordinate the pool name. 
        // important note: to use this, the T type must either include the AZ_RTTI macro, or be externally exposed via AZ_TYPE_INFO_SPECIALIZE
        template<class T>
        AZ::Outcome<AZStd::shared_ptr<InstancePool<T>>, AZStd::string> CreatePool(typename InstancePool<T>::ResetInstance resetFunc)
        {
            const char* name = AZ::AzTypeInfo<T>::Name();
            return CreatePool<T>(AZ::Name(name), resetFunc);
        }

        // returns the pool with the specified name
        template<class T>
        AZStd::shared_ptr<InstancePool<T>> GetPool(AZ::Name name)
        {
            auto foundIt = m_nameToInstancePool.find(name);
            if (foundIt != m_nameToInstancePool.end())
            {
                AZStd::shared_ptr<InstancePoolBase> instancePoolBase = foundIt->second.lock();
                return AZStd::rtti_pointer_cast<InstancePool<T>>(instancePoolBase);
            }
            return nullptr;
        }

        // same as above, but uses the AzTypeInfo name for the given T type
        template<class T>
        AZStd::shared_ptr<InstancePool<T>> GetPool()
        {
            const char* name = AZ::AzTypeInfo<T>::Name();
            return GetPool<T>(AZ::Name(name));
        }

    private:
        AZStd::unordered_map<AZ::Name, AZStd::weak_ptr<InstancePoolBase>> m_nameToInstancePool;
    };
} // namespace AZ

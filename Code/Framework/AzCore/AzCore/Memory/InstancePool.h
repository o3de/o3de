/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class InstancePoolBase
    {
    };

    template<typename T>
    class InstancePool : public InstancePoolBase
    {
        using ResetInstance = AZStd::function<void(T&)>;
        InstancePool(ResetInstance resetFunctor)
            : m_resetFunction(resetFunctor)
        {
        }

        T* GetInstance()
        {
            if (m_instances.empty())
            {
                auto fromList = m_instances.back();
                m_instances.pop_back();
                return fromList.release();
            }
            else
            {
                return new T;
            }
        }

        void RecycleInstance(AZStd::unique_ptr<T> instanceToRecycle)
        {
            if (instanceToRecycle == nullptr)
            {
                return;
            }
            if (m_resetFunction)
            {
                m_resetFunction(*instanceToRecycle);
            }

            m_instances.emplace_back(AZStd::move(instanceToRecycle));
        }

    private:
        AZStd::vector<AZStd::unique_ptr<T>> m_instances;
        ResetInstance m_resetFunction;
    };

    class PoolManagerInterface
    {
    public:
        AZ_RTTI(PoolManagerInterface, "{58980615-BC01-470F-8056-D2E2EFDFF27E}");
    };

    class PoolManager : public PoolManagerInterface
    {
    public:
        PoolManager()
        {
            AZ::Interface<PoolManagerInterface>::Register(this);
        }

        virtual ~PoolManager()
        {
            AZ::Interface<PoolManagerInterface>::Unregister(this);
        }

        template<class T, class... Args>
        AZ::Outcome<void, AZStd::string> EmplacePool(AZ::Name name, Args&&... args)
        {
            if (auto foundIt = m_nameToInstancePool.find(name); foundIt != m_nameToInstancePool.end())
            {
                return AZ::Failure(AZStd::string::format("Instance already exist with name %.s", name.GetCStr()));
            }

            AZStd::shared_ptr<InstancePool<T>> instancePool = AZStd::make_shared<InstancePool<T>>(AZStd::forward<Args>(args)...);
            m_nameToInstancePool.emplace(name, instancePool);
            return AZ::Success();
        }

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

        template<class T>
        AZStd::shared_ptr<InstancePool<T>> GetPool()
        {
            const char* name = AZ::AzTypeInfo<T>::Name();
            auto foundIt = m_nameToInstancePool.find(AZ::Name(name));
            if (foundIt != m_nameToInstancePool.end())
            {
                AZStd::shared_ptr<InstancePoolBase> instancePoolBase = foundIt->second.lock();
                return AZStd::rtti_pointer_cast<InstancePool<T>>(instancePoolBase);
            }

            return nullptr;
        }

    private:
        AZStd::unordered_map<AZ::Name, AZStd::weak_ptr<InstancePoolBase>> m_nameToInstancePool;
    };
} // namespace AZ

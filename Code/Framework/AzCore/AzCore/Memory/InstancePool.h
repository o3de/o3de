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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/utils.h>

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

        T* GetInstance()
        {
            /*if (!m_instances.empty())
            {
                T* fromList = m_instances.back().release();
                m_instances.pop_back();
                return fromList;
            }
            else*/
            {
                return new T;
            }
        }

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

        template<class T>
        AZ::Outcome<AZStd::shared_ptr<InstancePool<T>>, AZStd::string> CreatePool(typename InstancePool<T>::ResetInstance resetFunc)
        {
            const char* name = AZ::AzTypeInfo<T>::Name();
            return CreatePool<T>(AZ::Name(name), resetFunc);
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
            return GetPool<T>(AZ::Name(name));
        }

    private:
        AZStd::unordered_map<AZ::Name, AZStd::weak_ptr<InstancePoolBase>> m_nameToInstancePool;
    };
} // namespace AZ

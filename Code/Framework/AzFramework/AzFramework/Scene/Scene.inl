/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AzFramework
{
    template<typename T>
    bool Scene::SetSubsystem(T&& system)
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        for (const AZ::TypeId& key : m_systemKeys)
        {
            if (key == targetType)
            {
                return false;
            }
        }

        m_systemKeys.push_back(targetType);
        m_systemObjects.emplace_back(AZStd::forward<T>(system));
        m_subsystemEvent.Signal(*this, SubsystemEventType::Added, targetType);
        return true;
    }

    template<typename T>
    bool Scene::UnsetSubsystem()
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        const size_t m_systemKeysCount = m_systemKeys.size();
        for (size_t i = 0; i < m_systemKeysCount; ++i)
        {
            if (m_systemKeys[i] != targetType)
            {
                continue;
            }
            else
            {
                RemoveSubsystem(i, targetType);
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool Scene::UnsetSubsystem([[maybe_unused]] const T& system)
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        const size_t systemKeysCount = m_systemKeys.size();
        for (size_t i = 0; i < systemKeysCount; ++i)
        {
            if (m_systemKeys[i] != targetType)
            {
                continue;
            }
            else
            {
                [[maybe_unused]] T* instance = AZStd::any_cast<T>(&m_systemObjects[i]);
                AZ_Assert(
                    instance && *instance == system,
                    "Subsystem being released matched type, but wasn't pointing to the same system that was stored.");
                RemoveSubsystem(i, targetType);
                return true;
            }
        }
        return false;
    }

    template<typename T>
    T* Scene::FindSubsystem()
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        AZStd::any* subSystem = FindSubsystem(targetType);
        return subSystem ? AZStd::any_cast<T>(subSystem) : nullptr;
    }

    template<typename T>
    const T* Scene::FindSubsystem() const
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        const AZStd::any* subSystem = FindSubsystem(targetType);
        return subSystem ? AZStd::any_cast<const T>(subSystem) : nullptr;
    }

    template<typename T>
    T* Scene::FindSubsystemInScene()
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        AZStd::any* subSystem = FindSubsystemInScene(targetType);
        return subSystem ? AZStd::any_cast<T>(subSystem) : nullptr;
    }

    template<typename T>
    const T* Scene::FindSubsystemInScene() const
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        const AZStd::any* subSystem = FindSubsystemInScene(targetType);
        return subSystem ? AZStd::any_cast<const T>(subSystem) : nullptr;
    }
} // namespace AzFramework

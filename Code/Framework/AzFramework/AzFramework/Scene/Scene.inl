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

namespace AzFramework
{
    template<typename T>
    bool Scene::SetSubsystem(T&& system)
    {
        const AZ::TypeId& targetType = azrtti_typeid<T>();
        const size_t m_systemKeysCount = m_systemKeys.size();
        for (size_t i = 0; i < m_systemKeysCount; ++i)
        {
            if (m_systemKeys[i] == targetType)
            {
                return false;
            }
        }

        m_systemKeys.push_back(targetType);
        m_systemObjects.emplace_back(AZStd::forward<T>(system));
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
                m_systemKeys[i] = m_systemKeys.back();
                m_systemObjects[i] = AZStd::move(m_systemObjects.back());
                m_systemKeys.pop_back();
                m_systemObjects.pop_back();
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool Scene::UnsetSubsystem(const T& system)
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
                T* instance = AZStd::any_cast<T>(&m_systemObjects[i]);
                if (instance && *instance == system)
                {
                    m_systemKeys[i] = m_systemKeys.back();
                    m_systemObjects[i] = AZStd::move(m_systemObjects.back());
                    m_systemKeys.pop_back();
                    m_systemObjects.pop_back();
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
        return false;
    }

    template<typename T>
    T* Scene::FindSubsystem()
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
                return AZStd::any_cast<T>(&m_systemObjects[i]);
            }
        }
        return nullptr;
    }

    template<typename T>
    const T* Scene::FindSubsystem() const
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
                return AZStd::any_cast<const T>(&m_systemObjects[i]);
            }
        }
        return nullptr;
    }
} // namespace AzFramework

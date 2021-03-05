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
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

namespace AzFramework
{
    class Scene final
    {
    public:
        AZ_TYPE_INFO(Scene, "{DB449BB3-7A95-434D-BC61-47ACBB1F3436}");
        AZ_CLASS_ALLOCATOR(Scene, AZ::SystemAllocator, 0);

        explicit Scene(AZStd::string_view name);

        const AZStd::string& GetName();
        
        // Set the instance of a subsystem associated with this scene.
        template <typename T>
        bool SetSubsystem(T* system);

        // Unset the instance of a subsystem associated with this scene.
        template <typename T>
        bool UnsetSubsystem();

        // Get the instance of a subsystem associated with this scene.
        template <typename T>
        T* GetSubsystem();

    private:

        AZStd::string m_name;

        // Storing keys separate from data to optimize for fast key search.
        AZStd::vector<AZ::TypeId> m_systemKeys;
        AZStd::vector<void*> m_systemPointers;
    };
    
    template <typename T>
    bool Scene::SetSubsystem(T* system)
    {
        if (GetSubsystem<T>() != nullptr)
        {
            return false;
        }
        m_systemKeys.push_back(T::RTTI_Type());
        m_systemPointers.push_back(system);
        return true;
    }

    template <typename T>
    bool Scene::UnsetSubsystem()
    {
        for (size_t i = 0; i < m_systemKeys.size(); ++i)
        {
            if (m_systemKeys.at(i) == T::RTTI_Type())
            {
                m_systemKeys.at(i) = m_systemKeys.back();
                m_systemKeys.pop_back();
                m_systemPointers.at(i) = m_systemPointers.back();
                m_systemPointers.pop_back();
                return true;
            }
        }
        return false;
    }

    template <typename T>
    T* Scene::GetSubsystem()
    {
        for (size_t i = 0; i < m_systemKeys.size(); ++i)
        {
            if (m_systemKeys.at(i) == T::RTTI_Type())
            {
                return reinterpret_cast<T*>(m_systemPointers.at(i));
            }
        }
        return nullptr;
    }

} // AzFramework
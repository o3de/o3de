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
#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string_view.h>

namespace AzFramework
{
    class Scene final
    {
    public:
        AZ_TYPE_INFO(Scene, "{DB449BB3-7A95-434D-BC61-47ACBB1F3436}");
        AZ_CLASS_ALLOCATOR(Scene, AZ::SystemAllocator, 0);

        constexpr static AZStd::string_view MainSceneName = "Main";
        constexpr static AZStd::string_view EditorMainSceneName = "Editor";

        explicit Scene(AZStd::string name);
        Scene(AZStd::string name, AZStd::shared_ptr<Scene> parent);

        const AZStd::string& GetName() const;
        
        // Set the instance of a subsystem associated with this scene.
        template <typename T>
        bool SetSubsystem(T&& system);

        // Unset the instance of a subsystem associated with this scene.
        template <typename T>
        bool UnsetSubsystem();

        // Unset the instance of the exact system associated with this scene.
        // Use this to make sure the expected instance is removed or to make sure type deduction is done in the same was as during setting.
        template<typename T>
        bool UnsetSubsystem(const T& system);

        // Get the instance of a subsystem associated with this scene.
        template <typename T>
        T* FindSubsystem();

        // Get the instance of a subsystem associated with this scene.
        template<typename T>
        const T* FindSubsystem() const;

    private:
        // Storing keys separate from data to optimize for fast key search.
        AZStd::vector<AZ::TypeId> m_systemKeys;
        AZStd::vector<AZStd::any> m_systemObjects;

        // Name that identifies the scene.
        AZStd::string m_name;
        // Parent to this scene. Any subsystems are inherited from the parent but can be overwritten locally.
        AZStd::shared_ptr<Scene> m_parent;
    };
} // AzFramework

#include <AzFramework/Scene/Scene.inl>

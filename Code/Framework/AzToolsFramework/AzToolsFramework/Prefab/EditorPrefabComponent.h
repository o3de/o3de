/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        struct EditorPrefabOverride final
        {
            AZ_RTTI(EditorPrefabOverride, "{348F250E-1EA9-4AA0-A02F-9D32D9B9B585}");
            AZ_CLASS_ALLOCATOR(EditorPrefabOverride, AZ::SystemAllocator);

            EditorPrefabOverride() = default;
            EditorPrefabOverride(AZStd::string label, AZStd::string value);

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_label;
            AZStd::string m_value;
        };

        class EditorPrefabComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            static inline constexpr AZ::TypeId EditorPrefabComponentTypeId{ "{756E5F9C-3E08-4F8D-855C-A5AEEFB6FCDD}" };

            AZ_COMPONENT(EditorPrefabComponent, EditorPrefabComponentTypeId, EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        private:
            void Activate() override;
            void Deactivate() override;

            AZStd::vector<EditorPrefabOverride> m_patches;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

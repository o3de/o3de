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
        class EditorPrefabComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            static inline constexpr AZ::TypeId EditorPrefabComponentTypeId{ "{756E5F9C-3E08-4F8D-855C-A5AEEFB6FCDD}" };

            AZ_COMPONENT(EditorPrefabComponent, EditorPrefabComponentTypeId, EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            AZStd::string GetLabelForIndex(int index) const;

        private:
            void Activate() override;
            void Deactivate() override;

            // Overrides UI
            struct EditorPrefabOverride final
            {
                AZ_RTTI(EditorPrefabOverride, "{56D1C6B9-7096-4CD5-8E18-66330A43E0E1}");
                AZ_CLASS_ALLOCATOR(EditorPrefabOverride, AZ::SystemAllocator);

                EditorPrefabOverride() = default;
                EditorPrefabOverride(AZ::IO::Path path, AZStd::string value);

                static void Reflect(AZ::ReflectContext* context);

                AZStd::string GetPropertyName() const;
                AZStd::string GetPropertyPath() const;

                AZ::IO::Path m_path;
                AZStd::string m_value;
            };

            //void GenerateOverridesList();

            // TODO - Ensure this does not get serialized to JSON.
            // This is just an Editor component and the data shown is instance-dependent.
            AZStd::vector<EditorPrefabOverride> m_overrides;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

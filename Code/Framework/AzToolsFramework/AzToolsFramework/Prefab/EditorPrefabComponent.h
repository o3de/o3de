/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class EditorPrefabComponent : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            static constexpr const char* const EditorPrefabComponentTypeId = "{756E5F9C-3E08-4F8D-855C-A5AEEFB6FCDD}";

            AZ_COMPONENT(EditorPrefabComponent, EditorPrefabComponentTypeId, EditorComponentBase);

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        private:
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    class EditorEntityModel;

    namespace Components
    {
        class AZTF_API EditorEntityModelComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(EditorEntityModelComponent, "{DD029F2E-63E0-41BD-A6BE-B447FDF11A60}")
            
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            EditorEntityModelComponent();
            ~EditorEntityModelComponent();
            EditorEntityModelComponent(const EditorEntityModelComponent&) = delete;
            EditorEntityModelComponent& operator=(const EditorEntityModelComponent&) = delete;

            void Activate() override;
            void Deactivate() override;

        private:
            AZStd::unique_ptr<EditorEntityModel> m_entityModel;
        };

    } // namespace Components
} // namespace AzToolsFramework

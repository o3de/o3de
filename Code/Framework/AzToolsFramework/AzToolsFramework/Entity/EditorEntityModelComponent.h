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

#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    class EditorEntityModel;

    namespace Components
    {
        class EditorEntityModelComponent
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

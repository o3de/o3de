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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponentMode.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>

namespace AzToolsFramework
{
    namespace Components
    {
        //! Allows working with non-uniform scale in the editor.
        class EditorNonUniformScaleComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public AZ::NonUniformScaleRequestBus::Handler
            , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , public AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorNonUniformScaleComponent, "{2933FB4F-B3DA-4CD1-8106-F37300730777}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);

            EditorNonUniformScaleComponent() = default;
            ~EditorNonUniformScaleComponent() = default;

            // AZ::Component ...
            void Activate() override;
            void Deactivate() override;

            // AZ::NonUniformScaleRequestBus::Handler ...
            AZ::Vector3 GetScale() const override;
            void SetScale(const AZ::Vector3& scale) override;
            void RegisterScaleChangedEvent(AZ::NonUniformScaleChangedEvent::Handler& handler);

        private:
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

            void OnScaleChanged();

            // EditorComponentBase ...
            void BuildGameEntity(AZ::Entity* gameEntity) override;

            AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleChangedEvent m_scaleChangedEvent;

            //! Responsible for detecting ComponentMode activation and creating a concrete ComponentMode.
            AzToolsFramework::ComponentModeFramework::ComponentModeDelegate m_componentModeDelegate;
        };
    } // namespace Components
} // namespace AzToolsFramework

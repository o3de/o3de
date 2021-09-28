/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            void RegisterScaleChangedEvent(AZ::NonUniformScaleChangedEvent::Handler& handler) override;

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

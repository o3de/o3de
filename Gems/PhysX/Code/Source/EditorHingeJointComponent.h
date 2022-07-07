
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>

#include <PhysX/EditorJointBus.h>
#include <Editor/EditorJointConfiguration.h>
#include <Source/EditorJointComponent.h>

namespace PhysX
{
    class EditorHingeJointComponent
        : public EditorJointComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorHingeJointComponent, "{AF60FD48-4ADC-4C8C-8D3A-A4F7AE63C74D}", EditorJointComponent);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        // PhysX::EditorJointRequests
        float GetLinearValue(const AZStd::string& parameterName) override;
        AngleLimitsFloatPair GetLinearValuePair(const AZStd::string& parameterName) override;
        AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> GetSubComponentModesState() override;
        void SetBoolValue(const AZStd::string& parameterName, bool value) override;
        void SetLinearValue(const AZStd::string& parameterName, float value) override;
        void SetLinearValuePair(const AZStd::string& parameterName, const AngleLimitsFloatPair& valuePair) override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        bool m_angularLimitFlag = false;
        EditorJointLimitPairConfig m_angularLimit;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                       ///< and creating a concrete ComponentMode(s).
    };
} // namespace PhysX

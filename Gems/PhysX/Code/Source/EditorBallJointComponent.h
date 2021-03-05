
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

#include <AzFramework/Physics/Joint.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>

#include <PhysX/EditorJointBus.h>
#include <Editor/EditorJointConfiguration.h>
#include <Source/EditorJointComponent.h>

namespace PhysX
{
    class EditorBallJointComponent
        : public EditorJointComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorBallJointComponent, "{3D770685-9271-444D-B8AE-783B652C0986}", EditorJointComponent);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        // PhysX::EditorJointRequests
        float GetLinearValue(const AZStd::string& parameterName) override;
        AngleLimitsFloatPair GetLinearValuePair(const AZStd::string& parameterName) override;
        bool IsParameterUsed(const AZStd::string& parameterName) override;
        void SetBoolValue(const AZStd::string& parameterName, bool value) override;
        void SetLinearValue(const AZStd::string& parameterName, float value) override;
        void SetLinearValuePair(const AZStd::string& parameterName, const AngleLimitsFloatPair& valuePair) override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        bool m_swingIsLimited = false;
        EditorJointLimitConeConfig m_swingLimit;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation
                                                       ///< and creating a concrete ComponentMode(s).
    };
} // namespace PhysX

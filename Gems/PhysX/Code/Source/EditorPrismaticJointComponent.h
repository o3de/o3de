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

#include <PhysX/EditorJointBus.h>
#include <Editor/EditorJointConfiguration.h>
#include <Source/EditorJointComponent.h>

namespace PhysX
{
    class EditorPrismaticJointComponent
        : public EditorJointComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorPrismaticJointComponent, "{607B246E-C2DB-4D43-ABFA-A5A3994867C5}", EditorJointComponent);
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
        LinearLimitsFloatPair GetLinearValuePair(const AZStd::string& parameterName) override;
        AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> GetSubComponentModesState() override;
        void SetBoolValue(const AZStd::string& parameterName, bool value) override;
        void SetLinearValue(const AZStd::string& parameterName, float value) override;
        void SetLinearValuePair(const AZStd::string& parameterName, const LinearLimitsFloatPair& valuePair) override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        bool m_linearLimitFlag = false;
        EditorJointLimitLinearPairConfig m_linearLimit;
    };
} // namespace PhysX

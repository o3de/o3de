
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Visibility/BoundsBus.h>

#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <PhysX/EditorJointBus.h>
#include <Editor/EditorJointConfiguration.h>

namespace PhysX
{
    /// Base class for editor joint components.
    class EditorJointComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , protected AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
        , protected PhysX::EditorJointRequestBus::Handler
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorJointComponent, "{070CF18E-E328-43A6-9B76-F160CCD64B72}");
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

    protected:
        // EditorComponentSelectionRequestsBus overrides ....
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override { return true; };

        // EditorJointRequestBus overrides ...
        bool GetBoolValue(const AZStd::string& parameterName) override;
        AZ::EntityId GetEntityIdValue(const AZStd::string& parameterName) override;
        float GetLinearValue(const AZStd::string& parameterName) override;
        AngleLimitsFloatPair GetLinearValuePair(const AZStd::string& parameterName) override;
        AZ::Transform GetTransformValue(const AZStd::string& parameterName) override;
        AZ::Vector3 GetVector3Value(const AZStd::string& parameterName) override;
        AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> GetSubComponentModesState() override;
        void SetBoolValue(const AZStd::string& parameterName, bool value) override;
        void SetEntityIdValue(const AZStd::string& parameterName, AZ::EntityId value) override;
        void SetLinearValue(const AZStd::string& parameterName, float value) override;
        void SetLinearValuePair(const AZStd::string& parameterName, const AngleLimitsFloatPair& valuePair) override;
        void SetVector3Value(const AZStd::string& parameterName, const AZ::Vector3& value) override;

        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        EditorJointConfig m_config;
    };
} // namespace PhysX

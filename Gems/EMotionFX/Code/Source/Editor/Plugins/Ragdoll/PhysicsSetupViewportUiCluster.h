/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <Editor/Plugins/Ragdoll/PhysicsSetupManipulators.h>

namespace EMotionFX
{
    class Actor;
    class Node;
    class Transform;

    //! Provides UI in the viewport for manipulating physics configurations such as collider and joint limit settings.
    class PhysicsSetupViewportUiCluster
    {
    public:
        PhysicsSetupViewportUiCluster();
        void UpdateClusters(PhysicsSetupManipulatorData physicsSetupManipulatorData);
        void DestroyClusterIfExists();

        enum class SubMode : AZ::u32
        {
            Null,
            ColliderTranslation,
            ColliderRotation,
            ColliderDimensions,
            JointLimitParentRotation,
            JointLimitChildRotation,
            JointSwingLimits,
            JointTwistLimits,
            JointLimitOptimization,
            NumModes
        };

        //! Used to track the cluster that a specific button is a part of.
        struct ButtonData
        {
            AzToolsFramework::ViewportUi::ClusterId m_clusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
            AzToolsFramework::ViewportUi::ButtonId m_buttonId = AzToolsFramework::ViewportUi::InvalidButtonId;
        };

        constexpr static const char* const ColliderTranslationTooltip = "Switch to collider translation mode";
        constexpr static const char* const ColliderRotationTooltip = "Switch to collider rotation mode";
        constexpr static const char* const ColliderDimensionsTooltip = "Switch to collider dimensions mode";
        constexpr static const char* const JointLimitParentRotationTooltip = "Switch to joint limit parent frame rotation mode";
        constexpr static const char* const JointLimitChildRotationTooltip = "Switch to joint limit child frame rotation mode";
        constexpr static const char* const JointLimitSwingTooltip = "Switch to joint swing limit mode";
        constexpr static const char* const JointLimitTwistTooltip = "Switch to joint twist limit mode";
        constexpr static const char* const JointLimitAutofitTooltip = "Automatic joint limit setup";

    private:
        void SetCurrentMode(SubMode mode);
        AZ::s32 GetViewportId() const;

        AzToolsFramework::ViewportUi::ClusterId m_colliderClusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        AzToolsFramework::ViewportUi::ClusterId m_jointLimitClusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        AZStd::vector<ButtonData> m_buttonData;
        AZStd::unordered_map<SubMode, AZStd::unique_ptr<PhysicsSetupManipulatorsBase>> m_subModes;
        SubMode m_subMode = SubMode::Null;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler
            m_colliderModeSelectionHandler; //!< Event handler for sub mode changes in the collider cluster.
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler
            m_jointLimitModeSelectionHandler; //!< Event handler for sub mode changes in the joint limit cluster.
        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        mutable AZStd::optional<AZ::s32> m_viewportId;
        bool m_hasColliders = false;
        bool m_hasCapsuleCollider = false;
        bool m_hasJointLimit = false;
    };
} // namespace EMotionFX

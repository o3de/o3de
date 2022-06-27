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
        void CreateClusterIfNoneExists(PhysicsSetupManipulatorData physicsSetupManipulatorData);
        void DestroyClusterIfExists();

        enum class SubMode : AZ::u32
        {
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

    private:
        void SetCurrentMode(SubMode mode);
        AZ::s32 GetViewportId() const;

        AzToolsFramework::ViewportUi::ClusterId m_clusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        AZStd::vector<AzToolsFramework::ViewportUi::ButtonId> m_buttonIds;
        AZStd::unordered_map<SubMode, AZStd::unique_ptr<PhysicsSetupManipulatorsBase>> m_subModes;
        SubMode m_subMode = SubMode::ColliderTranslation;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_modeSelectionHandler; //!< Event handler for sub mode changes.
        PhysicsSetupManipulatorData m_physicsSetupManipulatorData;
        mutable AZStd::optional<AZ::s32> m_viewportId;
        bool m_hasCapsuleCollider = false;
        bool m_hasJointLimit = false;
    };
} // namespace EMotionFX

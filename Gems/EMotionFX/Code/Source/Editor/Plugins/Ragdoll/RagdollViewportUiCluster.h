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
#include <Editor/Plugins/Ragdoll/RagdollManipulators.h>

namespace EMotionFX
{
    class Actor;
    class Node;
    class Transform;

    //! Provides UI in the viewport for manipulating ragdoll configurations such as collider and joint limit settings. 
    class RagdollViewportUiCluster
    {
    public:
        RagdollViewportUiCluster();
        void CreateClusterIfNoneExists(RagdollManipulatorData ragdollManipulatorData);
        void DestroyClusterIfExists();

        enum class SubMode : AZ::u32
        {
            ColliderTranslation,
            NumModes
        };

    private:
        void SetCurrentMode(SubMode mode);

        AzToolsFramework::ViewportUi::ClusterId m_clusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        AZStd::vector<AzToolsFramework::ViewportUi::ButtonId> m_buttonIds;
        AZStd::unordered_map<SubMode, AZStd::unique_ptr<RagdollManipulatorsBase>> m_subModes;
        SubMode m_subMode = SubMode::ColliderTranslation;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_modeSelectionHandler; //!< Event handler for sub mode changes.
        RagdollManipulatorData m_ragdollManipulatorData;
    };
} // namespace EMotionFX

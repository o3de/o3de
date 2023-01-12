/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeBus.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>

namespace AZ
{
    class EntityComponentIdPair;
}

namespace PhysX
{
    //! Class responsible for managing component mode for joints.
    class JointsComponentMode final
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , public PhysX::JointsComponentModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(JointsComponentMode, "{BF9ABF22-950B-4FDD-846D-643D27F449C5}", EditorBaseComponentMode)

        JointsComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~JointsComponentMode();

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        // JointsComponentModeRequestBus overrides ...
        void SetCurrentSubMode(JointsComponentModeCommon::SubComponentModes::ModeType newMode) override;
        void ResetCurrentSubMode() override;
        bool IsCurrentSubModeAvailable(JointsComponentModeCommon::SubComponentModes::ModeType mode) const override;

        // EditorBaseComponentMode ...
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> PopulateViewportUiImpl() override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;

    private:
        //! Used to identify the group of component modes.
        enum class ClusterGroups
        {
            Group1 = 0, //!< Position Joint, Rotate Joint, Snap Position, Snap Rotation.
            Group2, //!< Damping, Stiffness, Twist Limits, Swing Limits.
            Group3, //!< Max Force, Max Torque.

            GroupCount
        };

        //! Used to track the cluster that a specific button is apart of.
        struct ButtonData
        {
            AzToolsFramework::ViewportUi::ClusterId m_clusterId;
            AzToolsFramework::ViewportUi::ButtonId m_buttonId;
        };

        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void SetupSubModes(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void TeardownSubModes();

        AzToolsFramework::ViewportUi::ClusterId GetClusterId(ClusterGroups group);

        AZStd::fixed_vector<AzToolsFramework::ViewportUi::ClusterId, static_cast<int>(ClusterGroups::GroupCount)> m_modeSelectionClusterIds; //!< List of the cluster ui's. The sub modes are split across 3 groups and each group is it's own cluster ui.
        AZStd::unordered_map<JointsComponentModeCommon::SubComponentModes::ModeType, ButtonData> m_buttonData; //!< Mapping of joint component modes to the button data.

        AZStd::fixed_vector<AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler, static_cast<int>(ClusterGroups::GroupCount)>
            m_modeSelectionHandlers; //!< Input handlers for each cluster UI.
        ButtonData m_activeButton; //!< The current highlighted button data.

        JointsComponentModeCommon::SubComponentModes::ModeType m_subMode = JointsComponentModeCommon::SubComponentModes::ModeType::Translation; //!< The current component mode that is active.
        AZStd::unordered_map<JointsComponentModeCommon::SubComponentModes::ModeType, AZStd::unique_ptr<PhysXSubComponentModeBase>> m_subModes; //!< The logic handlers for each component mode.
    };
}

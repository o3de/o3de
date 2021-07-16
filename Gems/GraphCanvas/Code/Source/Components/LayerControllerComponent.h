/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/string/string.h>

#include <GraphCanvas/Utils/StateControllers/StackStateController.h>

#include <GraphCanvas/Components/LayerBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>

namespace GraphCanvas
{
    class StyleManagerRequests;

    class LayerUtils
    {
    public:
        static int AlwaysOnTopZValue();
        static int AlwaysOnBottomZValue();
    };

    class LayerControllerComponent
        : public AZ::Component
        , public StateController<AZStd::string>::Notifications::Handler
        , public RootGraphicsItemNotificationBus::Handler
        , public LayerControllerRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public SceneNotificationBus::Handler
        , public GroupableSceneMemberNotificationBus::MultiHandler
        , public AZ::SystemTickBus::Handler
    {
        friend class LayerUtils;

    protected:
        // Offsets that can be set to offset within an individual layer.
        // This will allow us to manually sort each element into its own display category, while still adhering to other layering rules.
        enum LayerOffset
        {
            OffsetIndexForce = -1,

            // Controls the relative layering of each element inside of a particular layer.
            // In reverse display order here(higher up means lower down in the z-order)
            ConnectionOffset,
            NodeOffset,
            NodeGroupOffset,
            CommentOffset,
            BookmarkAnchorOffset,

            OffsetCount
        };

        LayerControllerComponent(AZStd::string_view layeringElement, LayerOffset layerOffset);
        
    public:
        
        static void Reflect(AZ::ReflectContext* context);
        AZ_COMPONENT(LayerControllerComponent, "{A85BE3B4-18D5-45D4-91B2-B5529C999E3D}", AZ::Component);
        
        LayerControllerComponent();
        ~LayerControllerComponent() override = default;

        // Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SystemTickBus
        void OnSystemTick() override;
        ////
        
        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        ////
        
        // SceneNotificationsBus
        void OnStylesChanged() override;
        void OnSelectionChanged() override;
        ////
        
        // RootGraphicsItemNotificationBus
        void OnDisplayStateChanged(RootGraphicsItemDisplayState oldState, RootGraphicsItemDisplayState newState) override;
        ////
        
        // StateController::Notifications
        void OnStateChanged(const AZStd::string& state) override;
        ////

        // LayerControllerRequestBus
        StateController<AZStd::string>* GetLayerModifierController() override;

        int GetSelectionOffset() const override;
        int GetGroupLayerOffset() const override;
        ////

        // GroupableSceneMemberNotifications
        void OnGroupChanged() override;
        ////

        int GetLayerOffset() const;

    protected:

        void SetBaseModifier(AZStd::string_view baseModifier);

        void SetGroupLayerOffset(int groupOffset);
        void SetSelectionLayerOffset(int selectionOffset);
    
    private:

        int CalculateZValue(int layer);
        void UpdateZValue();
        
        void ComputeCurrentLayer();

        int m_layer = 0;
        int m_layerOffset = 0;

        int m_selectionOffset = 0;
        int m_groupLayerOffset = 0;

        bool m_isInspected = false;

        GroupableSceneMemberRequests* m_groupableRequests = nullptr;
        SceneMemberUIRequests* m_uiRequests = nullptr;
        
        AZStd::string m_baseModifier;
        StackStateController< AZStd::string > m_externalLayerModifier;
        AZStd::string m_baseLayering;
        
        AZStd::string m_currentStyle;
        
        EditorId m_editorId;
    };
}

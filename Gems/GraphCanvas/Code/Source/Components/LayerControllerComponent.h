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

#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>

#include <GraphCanvas/Utils/StateControllers/StackStateController.h>

#include <GraphCanvas/Components/LayerBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>

namespace GraphCanvas
{
    class LayerControllerComponent
        : public AZ::Component
        , public StateController<AZStd::string>::Notifications::Handler
        , public RootGraphicsItemNotificationBus::Handler
        , public LayerControllerRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public SceneNotificationBus::Handler
    {
    protected:
        LayerControllerComponent(AZStd::string_view layeringElement);
        
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
        
        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        ////
        
        // SceneNotificationsBus
        void OnStylesChanged() override;
        ////
        
        // RootGraphicsItemNotificationBus
        void OnDisplayStateChanged(RootGraphicsItemDisplayState oldState, RootGraphicsItemDisplayState newState) override;
        ////
        
        // StateController::Notifications
        void OnStateChanged(const AZStd::string& state) override;
        ////

        // LayerControllerRequestBus
        StateController<AZStd::string>* GetLayerModifierController() override;
        ////

    protected:

        void SetBaseModifier(AZStd::string_view baseModifier);
    
    private:
        
        void ComputeCurrentLayer();
        
        AZStd::string m_baseModifier;
        StackStateController< AZStd::string > m_externalLayerModifier;
        AZStd::string m_baseLayering;
        
        AZStd::string m_currentStyle;
        
        EditorId m_editorId;
    };
}

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

#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Viewport/SingleViewportController.h>
#include <Atom/Viewport/InputController/MaterialEditorViewportInputControllerBus.h>
#include <Source/Viewport/InputController/Behavior.h>

namespace MaterialEditor
{
    class Behavior;

    //! Provides controls for manipulating camera, model, and environment in Material Editor
    class MaterialEditorViewportInputController
        : public AzFramework::SingleViewportController
        , public MaterialEditorViewportInputControllerRequestBus::Handler
    {
    public:

        AZ_TYPE_INFO(MaterialEditorViewportInputController, "{569A0544-7654-4DCE-8156-00A71B408374}");
        AZ_CLASS_ALLOCATOR(MaterialEditorViewportInputController, AZ::SystemAllocator, 0)

        MaterialEditorViewportInputController();
        virtual ~MaterialEditorViewportInputController();

        void Init(const AZ::EntityId& cameraEntityId, const AZ::EntityId& targetEntityId, const AZ::EntityId& iblEntityId);

        // MaterialEditorViewportInputControllerRequestBus::Handler interface overrides...
        const AZ::EntityId& GetCameraEntityId() const override;
        const AZ::EntityId& GetTargetEntityId() const override;
        const AZ::EntityId& GetIblEntityId() const override;
        const AZ::Vector3& GetTargetPosition() const override;
        void SetTargetPosition(const AZ::Vector3& targetPosition) override;
        float GetDistanceToTarget() const override;
        void GetExtents(float& distanceMin, float& distanceMax) const override;
        void Reset() override;
        void SetFieldOfView(float value) override;

        // AzFramework::ViewportControllerInstance interface overrides...
        bool HandleInputChannelEvent(AzFramework::ViewportId viewport, const AzFramework::InputChannel& inputChannel) override;
        void UpdateViewport(AzFramework::ViewportId viewport, AzFramework::FloatSeconds deltaTime, AZ::ScriptTimePoint time) override;

    private:
        using KeyMask = uint32_t;

        enum Keys
        {
            None = 0,
            Lmb = 1 << 0,
            Mmb = 1 << 1,
            Rmb = 1 << 2,
            Alt = 1 << 3,
            Ctrl = 1 << 4,
            Shift = 1 << 5
        };

        //! Calculate min and max dist and center based on mesh size of target model
        void CalculateExtents();
        //! Determine which behavior to set based on mouse/keyboard input
        void EvaluateControlBehavior();

        bool m_initialized = false;

        //! Input keys currently pressed
        KeyMask m_keys = None;
        //! Input key sequence changed
        bool m_keysChanged = false;
        //! Time remaining before behavior switch
        float m_timeToBehaviorSwitchMs = 0;

        //! Current behavior of the controller
        AZStd::shared_ptr<Behavior> m_behavior;
        AZStd::unordered_map<KeyMask, AZStd::shared_ptr<Behavior>> m_behaviorMap;

        AZ::EntityId m_cameraEntityId;
        //! Target entity is looking at
        AZ::EntityId m_targetEntityId;
        //! IBL entity for rotating environment lighting
        AZ::EntityId m_iblEntityId;
        //! Target position camera is pointed towards
        AZ::Vector3 m_targetPosition;
        //! Center of the model observed
        AZ::Vector3 m_modelCenter;
        //! Minimum distance from camera to target
        float m_distanceMin = 1.0f;
        //! Maximum distance from camera to target
        float m_distanceMax = 10.0f;

        static constexpr float MaxDistanceMultiplier = 2.5f;
        static constexpr float StartingDistanceMultiplier = 2.0f;
        static constexpr float StartingRotationAngle = AZ::Constants::QuarterPi / 2.0f;
        static constexpr float DepthNear = 0.01f;
        //! Artificial delay between behavior switching to avoid switching into undesired behaviors with smaller key sequences
        //! e.g. pressing RMB+LMB shouldn't switch into RMB behavior (or LMB behavior) first because it's virtually impossible to press both mouse buttons on the same frame
        static constexpr float BehaviorSwitchDelayMs = 0.1f;
    };
} // namespace MaterialEditor

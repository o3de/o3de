/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Viewport/SingleViewportController.h>

class QWidget;

namespace AtomToolsFramework
{
    class ViewportInputBehavior;

    //! Provides controls for manipulating camera, object, and environment in Material Editor
    class ViewportInputBehaviorController
        : public AzFramework::SingleViewportController
        , public ViewportInputBehaviorControllerInterface
    {
    public:
        AZ_TYPE_INFO(ViewportInputBehaviorController, "{569A0544-7654-4DCE-8156-00A71B408374}");
        AZ_CLASS_ALLOCATOR(ViewportInputBehaviorController, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(ViewportInputBehaviorController);

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

        ViewportInputBehaviorController(
            QWidget* owner,
            const AZ::EntityId& cameraEntityId,
            const AZ::EntityId& objectEntityId,
            const AZ::EntityId& environmentEntityId);
        virtual ~ViewportInputBehaviorController();

        void AddBehavior(KeyMask mask, AZStd::shared_ptr<ViewportInputBehavior> behavior);

        // ViewportInputBehaviorControllerInterface overrides...
        const AZ::EntityId& GetCameraEntityId() const override;
        const AZ::EntityId& GetObjectEntityId() const override;
        const AZ::EntityId& GetEnvironmentEntityId() const override;
        const AZ::Vector3& GetObjectPosition() const override;
        void SetObjectPosition(const AZ::Vector3& objectPosition) override;
        float GetObjectRadiusMin() const override;
        float GetObjectRadius() const override;
        float GetObjectDistance() const override;
        void Reset() override;
        void SetFieldOfView(float value) override;
        bool IsCameraCentered() const override;

        // AzFramework::ViewportControllerInstance overrides...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

    private:
        //! Determine which behavior to set based on mouse/keyboard input
        void EvaluateControlBehavior();

        //! Input keys currently pressed
        KeyMask m_keys = None;
        //! Input key sequence changed
        bool m_keysChanged = {};
        //! Time remaining before behavior switch
        float m_timeToBehaviorSwitchMs = {};

        //! Current behavior of the controller
        AZStd::shared_ptr<ViewportInputBehavior> m_behavior;
        AZStd::unordered_map<KeyMask, AZStd::shared_ptr<ViewportInputBehavior>> m_behaviorMap;

        AZ::EntityId m_cameraEntityId;
        //! Object camera is looking at
        AZ::EntityId m_objectEntityId;
        //! IBL entity for rotating environment lighting
        AZ::EntityId m_environmentEntityId;
        //! Object position camera is pointed towards
        AZ::Vector3 m_objectPosition = AZ::Vector3::CreateZero();
        //! Object bounds
        AZ::Aabb m_objectBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
        //! True if camera is centered on an object
        bool m_isCameraCentered = true;

        static constexpr float MaxDistanceMultiplier = 2.5f;
        static constexpr float StartingDistanceMultiplier = 2.0f;
        static constexpr float StartingRotationAngle = AZ::Constants::QuarterPi / 2.0f;
        static constexpr float DepthNear = 0.01f;
        //! Artificial delay between behavior switching to avoid switching into undesired behaviors with smaller key sequences
        //! e.g. pressing RMB+LMB shouldn't switch into RMB behavior (or LMB behavior) first because it's virtually impossible to press both
        //! mouse buttons on the same frame
        static constexpr float BehaviorSwitchDelayMs = 0.1f;
        QWidget* m_owner = {};
    };
} // namespace AtomToolsFramework

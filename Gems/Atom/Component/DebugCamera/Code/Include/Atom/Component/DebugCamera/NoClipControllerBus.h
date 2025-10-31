/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class ReflectContext;

    namespace Debug
    {
        enum NoClipControllerChannel : uint32_t
        {
            NoClipControllerChannel_None = 0x0,
            NoClipControllerChannel_Position = 0x1,
            NoClipControllerChannel_Orientation = 0x2,
            NoClipControllerChannel_Fov = 0x4,
        };

        struct NoClipControllerProperties
        {
            AZ_TYPE_INFO(NoClipControllerProperties, "{65F7E522-7FDE-414E-AA0F-638B234699B8}");

            static void Reflect(AZ::ReflectContext* context);

            float m_mouseSensitivityX = 1.0f;
            float m_mouseSensitivityY = 1.0f;
            float m_moveSpeed = 1.0f;
            float m_panningSpeed = 1.0f;
            float m_touchSensitivity = 3.0f;
        };

        class NoClipControllerRequests
            : public AZ::ComponentBus
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual void SetMouseSensitivityX(float mouseSensitivityX) = 0;
            virtual void SetMouseSensitivityY(float mouseSensitivityY) = 0;
            virtual void SetMoveSpeed(float moveSpeed) = 0;
            virtual void SetPanningSpeed(float panningSpeed) = 0;
            virtual void SetTouchSensitivity(float touchSensitivity) = 0;
            virtual void SetControllerProperties(const NoClipControllerProperties& properties) = 0;

            virtual void SetPosition(AZ::Vector3 position) = 0;
            virtual void SetHeading(float heading) = 0;
            virtual void SetPitch(float pitch) = 0;
            virtual void SetFov(float fov) = 0;

            virtual void SetCameraStateForward(float value) = 0;
            virtual void SetCameraStateBack(float value) = 0;
            virtual void SetCameraStateLeft(float value) = 0;
            virtual void SetCameraStateRight(float value) = 0;
            virtual void SetCameraStateUp(float value) = 0;
            virtual void SetCameraStateDown(float value) = 0;

            virtual float GetMouseSensitivityX() = 0;
            virtual float GetMouseSensitivityY() = 0;
            virtual float GetMoveSpeed() = 0;
            virtual float GetPanningSpeed() = 0;
            virtual float GetTouchSensitivity() = 0;
            virtual NoClipControllerProperties GetControllerProperties() = 0;

            virtual AZ::Vector3 GetPosition() = 0;
            virtual float GetHeading() = 0;
            virtual float GetPitch() = 0;
            virtual float GetFov() = 0;
        };

        using NoClipControllerRequestBus = AZ::EBus<NoClipControllerRequests>;
    } // namespace Debug
} // namespace AZ

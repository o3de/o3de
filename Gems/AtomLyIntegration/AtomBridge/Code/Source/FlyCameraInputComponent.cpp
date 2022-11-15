/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "FlyCameraInputComponent.h"

#include <ISystem.h>
#include <IConsole.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

#include <MathConversion.h>

#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/View.h>
#include <AzFramework/Components/CameraBus.h>

using namespace AzFramework;
using namespace AZ::AtomBridge;

//////////////////////////////////////////////////////////////////////////////
namespace
{
    //////////////////////////////////////////////////////////////////////////
    int GenerateThumbstickTexture()
    {
        // [GFX TODO] Get Atom test fly cam virtual thumbsticks working on mobile
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void ReleaseThumbstickTexture([[maybe_unused]] int textureId)
    {
        // [GFX TODO] Get Atom test fly cam virtual thumbsticks working on mobile
    }

    //////////////////////////////////////////////////////////////////////////
    void DrawThumbstick([[maybe_unused]] Vec2 initialPosition,
        [[maybe_unused]] Vec2 currentPosition,
        [[maybe_unused]] int textureId)
    {
        // [GFX TODO] Get Atom test fly cam virtual thumbsticks working on mobile
        // we do not have any 2D drawing capability like IDraw2d in Atom yet
    }
}

//////////////////////////////////////////////////////////////////////////////
const AZ::Crc32 FlyCameraInputComponent::UnknownInputChannelId("unknown_input_channel_id");

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
{
    required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("InputService", 0xd41af40c));
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Reflect(AZ::ReflectContext* reflection)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
    if (serializeContext)
    {
        serializeContext->Class<FlyCameraInputComponent, AZ::Component>()
            ->Version(1)
            ->Field("Move Speed", &FlyCameraInputComponent::m_moveSpeed)
            ->Field("Rotation Speed", &FlyCameraInputComponent::m_rotationSpeed)
            ->Field("Mouse Sensitivity", &FlyCameraInputComponent::m_mouseSensitivity)
            ->Field("Invert Rotation Input X", &FlyCameraInputComponent::m_InvertRotationInputAxisX)
            ->Field("Invert Rotation Input Y", &FlyCameraInputComponent::m_InvertRotationInputAxisY)
            ->Field("Is enabled", &FlyCameraInputComponent::m_isEnabled);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<FlyCameraInputComponent>("Fly Camera Input", "The Fly Camera Input allows you to control the camera")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute("Category", "Gameplay")
                ->Attribute("Icon", "Editor/Icons/Components/FlyCameraInput.svg")
                ->Attribute("ViewportIcon", "Editor/Icons/Components/Viewport/FlyCameraInput.svg")
                ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/gameplay/fly-camera-input/")
                ->Attribute("AutoExpand", true)
                ->Attribute("AppearsInAddComponentMenu", AZ_CRC("Game", 0x232b318c))
                ->DataElement(0, &FlyCameraInputComponent::m_moveSpeed, "Move Speed", "Speed at which the camera moves")
                ->Attribute("Min", 1.0f)
                ->Attribute("Max", 100.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_rotationSpeed, "Rotation Speed", "Speed at which the camera rotates")
                ->Attribute("Min", 1.0f)
                ->Attribute("Max", 100.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_mouseSensitivity, "Mouse Sensitivity", "Mouse sensitivity factor")
                ->Attribute("Min", 0.0f)
                ->Attribute("Max", 1.0f)
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_InvertRotationInputAxisX, "Invert Rotation Input X", "Invert rotation input x-axis")
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(0, &FlyCameraInputComponent::m_InvertRotationInputAxisY, "Invert Rotation Input Y", "Invert rotation input y-axis")
                ->Attribute("ChangeNotify", AZ_CRC("RefreshValues", 0x28e720d4))
                ->DataElement(AZ::Edit::UIHandlers::CheckBox, &FlyCameraInputComponent::m_isEnabled,
                    "Is Initially Enabled", "When checked, the fly cam input is enabled on activate, else it has to be specifically enabled.");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
    if (behaviorContext)
    {
        behaviorContext->EBus<FlyCameraInputBus>("FlyCameraInputBus")
            ->Event("SetIsEnabled", &FlyCameraInputBus::Events::SetIsEnabled)
            ->Event("GetIsEnabled", &FlyCameraInputBus::Events::GetIsEnabled);
    }
}

//////////////////////////////////////////////////////////////////////////////
FlyCameraInputComponent::~FlyCameraInputComponent()
{
    ReleaseThumbstickTexture(m_thumbstickTextureId);
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Init()
{
    m_thumbstickTextureId = GenerateThumbstickTexture();
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Activate()
{
    InputChannelEventListener::Connect();
    AZ::TickBus::Handler::BusConnect();
    FlyCameraInputBus::Handler::BusConnect(GetEntityId());
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::Deactivate()
{
    FlyCameraInputBus::Handler::BusDisconnect();
    AZ::TickBus::Handler::BusDisconnect();
    InputChannelEventListener::Disconnect();
}

//////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
{
    if (!m_isEnabled)
    {
        return;
    }

    AZ::Transform worldTransform = AZ::Transform::Identity();
    EBUS_EVENT_ID_RESULT(worldTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);

    // Update movement
    const float moveSpeed = m_moveSpeed * deltaTime;
    const AZ::Vector3 right = worldTransform.GetBasisX();
    const AZ::Vector3 forward = worldTransform.GetBasisY();
    const AZ::Vector3 movement = (forward * m_movement.y) + (right * m_movement.x);
    const AZ::Vector3 newPosition = worldTransform.GetTranslation() + (movement * moveSpeed);
    worldTransform.SetTranslation(newPosition);

    const Vec2 invertedRotation(m_InvertRotationInputAxisX ? m_rotation.x : -m_rotation.x,
        m_InvertRotationInputAxisY ? m_rotation.y : -m_rotation.y);

    // Update rotation (not sure how to do this properly using just AZ::Quaternion)
    const AZ::Quaternion worldOrientation = worldTransform.GetRotation();
    const Ang3 rotation(AZQuaternionToLYQuaternion(worldOrientation));
    const Ang3 newRotation = rotation + Ang3(DEG2RAD(invertedRotation.y), 0.f, DEG2RAD(invertedRotation.x)) * m_rotationSpeed;
    const AZ::Quaternion newOrientation = LYQuaternionToAZQuaternion(Quat(newRotation));
    worldTransform.SetRotation(newOrientation);

    EBUS_EVENT_ID(GetEntityId(), AZ::TransformBus, SetWorldTM, worldTransform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool FlyCameraInputComponent::OnInputChannelEventFiltered(const InputChannel& inputChannel)
{
    if (!m_isEnabled)
    {
        return false;
    }

    const InputDeviceId& deviceId = inputChannel.GetInputDevice().GetInputDeviceId();
    if (InputDeviceMouse::IsMouseDevice(deviceId))
    {
        OnMouseEvent(inputChannel);
    }
    else if (InputDeviceKeyboard::IsKeyboardDevice(deviceId))
    {
        OnKeyboardEvent(inputChannel);
    }
    else if (InputDeviceTouch::IsTouchDevice(deviceId))
    {
        const InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<InputChannel::PositionData2D>();
        if (positionData2D)
        {
            float defaultViewWidth = GetViewWidth();
            float defaultViewHeight = GetViewHeight();
            const Vec2 screenPosition(positionData2D->m_normalizedPosition.GetX() * defaultViewWidth,
                                      positionData2D->m_normalizedPosition.GetY() * defaultViewHeight);
            OnTouchEvent(inputChannel, screenPosition);
        }
    }
    else if (AzFramework::InputDeviceGamepad::IsGamepadDevice(deviceId))
    {
        OnGamepadEvent(inputChannel);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::SetIsEnabled(bool isEnabled)
{
    m_isEnabled = isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool FlyCameraInputComponent::GetIsEnabled()
{
    return m_isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float Snap_s360(float val)
{
    if (val < 0.0f)
    {
        val = f32(360.0f + fmodf(val, 360.0f));
    }
    else if (val >= 360.0f)
    {
        val = f32(fmodf(val, 360.0f));
    }
    return val;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnMouseEvent(const InputChannel& inputChannel)
{
    const InputChannelId& channelId = inputChannel.GetInputChannelId();
    if (channelId == InputDeviceMouse::Movement::X)
    {
        m_rotation.x = Snap_s360(inputChannel.GetValue() * m_mouseSensitivity);
    }
    else if (channelId == InputDeviceMouse::Movement::Y)
    {
        m_rotation.y = Snap_s360(inputChannel.GetValue() * m_mouseSensitivity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnKeyboardEvent(const InputChannel& inputChannel)
{
    if (gEnv && gEnv->pConsole && gEnv->pConsole->IsOpened())
    {
        return;
    }

    const InputChannelId& channelId = inputChannel.GetInputChannelId();
    if (channelId == InputDeviceKeyboard::Key::AlphanumericW)
    {
        m_movement.y = inputChannel.GetValue();
    }
    
    if (channelId == InputDeviceKeyboard::Key::AlphanumericA)
    {
        m_movement.x = -inputChannel.GetValue();
    }
    
    if (channelId == InputDeviceKeyboard::Key::AlphanumericS)
    {
        m_movement.y = -inputChannel.GetValue();
    }
    
    if (channelId == InputDeviceKeyboard::Key::AlphanumericD)
    {
        m_movement.x = inputChannel.GetValue();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnGamepadEvent(const InputChannel& inputChannel)
{
    const InputChannelId& channelId = inputChannel.GetInputChannelId();
    if (channelId == InputDeviceGamepad::ThumbStickAxis1D::LX)
    {
        m_movement.x = inputChannel.GetValue();
    }
    
    if (channelId == InputDeviceGamepad::ThumbStickAxis1D::LY)
    {
        m_movement.y = inputChannel.GetValue();
    }
    
    if (channelId == InputDeviceGamepad::ThumbStickAxis1D::RX)
    {
        m_rotation.x = inputChannel.GetValue();
    }
    
    if (channelId == InputDeviceGamepad::ThumbStickAxis1D::RY)
    {
        m_rotation.y = inputChannel.GetValue();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnTouchEvent(const InputChannel& inputChannel, const Vec2& screenPosition)
{
    if (inputChannel.IsStateBegan())
    {
        const float screenCentreX = GetViewWidth() * 0.5f;
        if (screenPosition.x <= screenCentreX)
        {
            if (m_leftFingerId == UnknownInputChannelId)
            {
                // Initiate left thumb-stick (movement)
                m_leftDownPosition = screenPosition;
                m_leftFingerId = inputChannel.GetInputChannelId().GetNameCrc32();
                DrawThumbstick(m_leftDownPosition, screenPosition, m_thumbstickTextureId);
            }
        }
        else
        {
            if (m_rightFingerId == UnknownInputChannelId)
            {
                // Initiate right thumb-stick (rotation)
                m_rightDownPosition = screenPosition;
                m_rightFingerId = inputChannel.GetInputChannelId().GetNameCrc32();
                DrawThumbstick(m_rightDownPosition, screenPosition, m_thumbstickTextureId);
            }
        }
    }
    else if (inputChannel.GetInputChannelId().GetNameCrc32() == m_leftFingerId)
    {
        // Update left thumb-stick (movement)
        OnVirtualLeftThumbstickEvent(inputChannel, screenPosition);
    }
    else if (inputChannel.GetInputChannelId().GetNameCrc32() == m_rightFingerId)
    {
        // Update right thumb-stick (rotation)
        OnVirtualRightThumbstickEvent(inputChannel, screenPosition);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnVirtualLeftThumbstickEvent(const InputChannel& inputChannel, const Vec2& screenPosition)
{
    if (inputChannel.GetInputChannelId().GetNameCrc32() != m_leftFingerId)
    {
        return;
    }

    switch (inputChannel.GetState())
    {
        case InputChannel::State::Ended:
        {
            // Stop movement
            m_leftFingerId = UnknownInputChannelId;
            m_movement = ZERO;
        }
        break;

        case InputChannel::State::Updated:
        {
            // Calculate movement
            const float discRadius = GetViewWidth() * m_virtualThumbstickRadiusAsPercentageOfScreenWidth;
            const float distScalar = 1.0f / discRadius;

            Vec2 dist = screenPosition - m_leftDownPosition;
            dist *= distScalar;

            m_movement.x = AZ::GetClamp(dist.x, -1.0f, 1.0f);
            m_movement.y = AZ::GetClamp(-dist.y, -1.0f, 1.0f);

            DrawThumbstick(m_leftDownPosition, screenPosition, m_thumbstickTextureId);
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FlyCameraInputComponent::OnVirtualRightThumbstickEvent(const InputChannel& inputChannel, const Vec2& screenPosition)
{
    if (inputChannel.GetInputChannelId().GetNameCrc32() != m_rightFingerId)
    {
        return;
    }

    switch (inputChannel.GetState())
    {
        case InputChannel::State::Ended:
        {
            // Stop rotation
            m_rightFingerId = UnknownInputChannelId;
            m_rotation = ZERO;
        }
        break;

        case InputChannel::State::Updated:
        {
            // Calculate rotation
            const float discRadius = GetViewWidth() * m_virtualThumbstickRadiusAsPercentageOfScreenWidth;
            const float distScalar = 1.0f / discRadius;

            Vec2 dist = screenPosition - m_rightDownPosition;
            dist *= distScalar;

            m_rotation.x = AZ::GetClamp(dist.x, -1.0f, 1.0f);
            m_rotation.y = AZ::GetClamp(dist.y, -1.0f, 1.0f);

            DrawThumbstick(m_rightDownPosition, screenPosition, m_thumbstickTextureId);
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float FlyCameraInputComponent::GetViewWidth() const
{
    float viewWidth = 256.0f;
    Camera::CameraRequestBus::EventResult(viewWidth, GetEntityId(), &Camera::CameraRequestBus::Events::GetFrustumWidth);
    return viewWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float FlyCameraInputComponent::GetViewHeight() const
{
    float viewHeight = 256.0f;
    Camera::CameraRequestBus::EventResult(viewHeight, GetEntityId(), &Camera::CameraRequestBus::Events::GetFrustumHeight);
    return viewHeight;
}


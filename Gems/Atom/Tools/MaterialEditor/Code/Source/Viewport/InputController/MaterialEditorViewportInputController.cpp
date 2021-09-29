/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>
#include <QWidget>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Matrix4x4.h>

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>

#include <Source/Viewport/InputController/MaterialEditorViewportInputController.h>
#include <Source/Viewport/InputController/IdleBehavior.h>
#include <Source/Viewport/InputController/PanCameraBehavior.h>
#include <Source/Viewport/InputController/OrbitCameraBehavior.h>
#include <Source/Viewport/InputController/MoveCameraBehavior.h>
#include <Source/Viewport/InputController/DollyCameraBehavior.h>
#include <Source/Viewport/InputController/RotateModelBehavior.h>
#include <Source/Viewport/InputController/RotateEnvironmentBehavior.h>

namespace MaterialEditor
{
    MaterialEditorViewportInputController::MaterialEditorViewportInputController()
        : AzFramework::SingleViewportController()
        , m_targetPosition(AZ::Vector3::CreateZero())
    {
        m_behaviorMap[None] = AZStd::make_shared<IdleBehavior>();
        m_behaviorMap[Lmb] = AZStd::make_shared<PanCameraBehavior>();
        m_behaviorMap[Mmb] = AZStd::make_shared<MoveCameraBehavior>();
        m_behaviorMap[Rmb] = AZStd::make_shared<OrbitCameraBehavior>();
        m_behaviorMap[Alt ^ Lmb] = AZStd::make_shared<OrbitCameraBehavior>();
        m_behaviorMap[Alt ^ Mmb] = AZStd::make_shared<MoveCameraBehavior>();
        m_behaviorMap[Alt ^ Rmb] = AZStd::make_shared<DollyCameraBehavior>();
        m_behaviorMap[Lmb ^ Rmb] = AZStd::make_shared<DollyCameraBehavior>();
        m_behaviorMap[Ctrl ^ Lmb] = AZStd::make_shared<RotateModelBehavior>();
        m_behaviorMap[Shift ^ Lmb] = AZStd::make_shared<RotateEnvironmentBehavior>();
    }

    MaterialEditorViewportInputController::~MaterialEditorViewportInputController()
    {
        if (m_initialized)
        {
            MaterialEditorViewportInputControllerRequestBus::Handler::BusDisconnect();
        }
    }

    void MaterialEditorViewportInputController::Init(const AZ::EntityId& cameraEntityId, const AZ::EntityId& targetEntityId, const AZ::EntityId& iblEntityId)
    {
        if (m_initialized)
        {
            AZ_Error("MaterialEditorViewportInputController", false, "Controller already initialized.");
            return;
        }
        m_initialized = true;
        m_cameraEntityId = cameraEntityId;
        m_targetEntityId = targetEntityId;
        m_iblEntityId = iblEntityId;

        MaterialEditorViewportInputControllerRequestBus::Handler::BusConnect();
    }

    const AZ::EntityId& MaterialEditorViewportInputController::GetCameraEntityId() const
    {
        return m_cameraEntityId;
    }

    const AZ::EntityId& MaterialEditorViewportInputController::GetTargetEntityId() const
    {
        return m_targetEntityId;
    }

    const AZ::EntityId& MaterialEditorViewportInputController::GetIblEntityId() const
    {
        return m_iblEntityId;
    }

    const AZ::Vector3& MaterialEditorViewportInputController::GetTargetPosition() const
    {
        return m_targetPosition;
    }

    void MaterialEditorViewportInputController::SetTargetPosition(const AZ::Vector3& targetPosition)
    {
        m_targetPosition = targetPosition;
        m_isCameraCentered = false;
    }

    float MaterialEditorViewportInputController::GetDistanceToTarget() const
    {
        AZ::Vector3 cameraPosition;
        AZ::TransformBus::EventResult(cameraPosition, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTranslation);
        return cameraPosition.GetDistance(m_targetPosition);
    }

    void MaterialEditorViewportInputController::GetExtents(float& distanceMin, float& distanceMax) const
    {
        distanceMin = m_distanceMin;
        distanceMax = m_distanceMax;
    }

    float MaterialEditorViewportInputController::GetRadius() const
    {
        return m_radius;
    }

    void MaterialEditorViewportInputController::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        if (m_keysChanged)
        {
            if (m_timeToBehaviorSwitchMs > 0)
            {
                m_timeToBehaviorSwitchMs -= event.m_deltaTime.count();
            }
            if (m_timeToBehaviorSwitchMs <= 0)
            {
                EvaluateControlBehavior();
                m_keysChanged = false;
            }
        }
    }

    bool MaterialEditorViewportInputController::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        using namespace AzFramework;

        const InputChannelId& inputChannelId = event.m_inputChannel.GetInputChannelId();
        const InputChannel::State state = event.m_inputChannel.GetState();
        const KeyMask keysOld = m_keys;

        bool mouseOver = false;
        AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::EventResult(
            mouseOver, GetViewportId(),
            &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::IsMouseOver);

        if (!m_behavior)
        {
            EvaluateControlBehavior();
        }

        switch (state)
        {
        case InputChannel::State::Began:
            if (inputChannelId == InputDeviceMouse::Button::Left)
            {
                m_keys |= Lmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Middle)
            {
                m_keys |= Mmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Right)
            {
                m_keys |= Rmb;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL)
            {
                m_keys |= Alt;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_keys |= Ctrl;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL)
            {
                m_keys |= Shift;
            }
            if (inputChannelId == InputDeviceMouse::Movement::X)
            {
                m_behavior->MoveX(event.m_inputChannel.GetValue());
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Y)
            {
                m_behavior->MoveY(event.m_inputChannel.GetValue());
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Z)
            {
                if (mouseOver)
                {
                    m_behavior->MoveZ(event.m_inputChannel.GetValue());
                }
            }
            break;
        case InputChannel::State::Ended:
            if (inputChannelId == InputDeviceMouse::Button::Left)
            {
                m_keys &= ~Lmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Middle)
            {
                m_keys &= ~Mmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Right)
            {
                m_keys &= ~Rmb;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL)
            {
                m_keys &= ~Alt;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_keys &= ~Ctrl;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL)
            {
                m_keys &= ~Shift;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::AlphanumericZ && (m_keys & Ctrl) == None)
            {
                // only reset camera if no other widget besides viewport is in focus
                const auto focus = QApplication::focusWidget();
                if (!focus || focus->objectName() == "Viewport")
                {
                    Reset();
                }
            }
            break;
        case InputChannel::State::Updated:
            if (inputChannelId == InputDeviceMouse::Movement::X)
            {
                m_behavior->MoveX(event.m_inputChannel.GetValue());
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Y)
            {
                m_behavior->MoveY(event.m_inputChannel.GetValue());
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Z)
            {
                if (mouseOver)
                {
                    m_behavior->MoveZ(event.m_inputChannel.GetValue());
                }
            }
            break;
        }

        if (keysOld != m_keys)
        {
            m_keysChanged = true;
            m_timeToBehaviorSwitchMs = BehaviorSwitchDelayMs;
        }
        return false;
    }

    void MaterialEditorViewportInputController::Reset()
    {
        CalculateExtents();

        // reset camera
        m_targetPosition = m_modelCenter;
        const float distance = m_distanceMin * StartingDistanceMultiplier;
        const AZ::Quaternion cameraRotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), StartingRotationAngle);
        AZ::Vector3 cameraPosition(m_targetPosition.GetX(), m_targetPosition.GetY() - distance, m_targetPosition.GetZ());
        cameraPosition = cameraRotation.TransformVector(cameraPosition);
        AZ::Transform cameraTransform = AZ::Transform::CreateFromQuaternionAndTranslation(cameraRotation, cameraPosition);
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTM, cameraTransform);
        m_isCameraCentered = true;

        // reset model
        AZ::Transform modelTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_targetEntityId, &AZ::TransformBus::Events::SetLocalTM, modelTransform);

        // reset environment
        AZ::Transform iblTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_iblEntityId, &AZ::TransformBus::Events::SetLocalTM, iblTransform);
        const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateIdentity();
        AZ::RPI::ScenePtr scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
        auto skyBoxFeatureProcessorInterface = scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
        skyBoxFeatureProcessorInterface->SetCubemapRotationMatrix(rotationMatrix);

        if (m_behavior)
        {
            m_behavior->End();
            m_behavior->Start();
        }
    }

    void MaterialEditorViewportInputController::SetFieldOfView(float value)
    {
        Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraRequestBus::Events::SetFovDegrees, value);
    }

    bool MaterialEditorViewportInputController::IsCameraCentered() const
    {
        return m_isCameraCentered;
    }

    void MaterialEditorViewportInputController::CalculateExtents()
    {
        AZ::TransformBus::EventResult(m_modelCenter, m_targetEntityId, &AZ::TransformBus::Events::GetLocalTranslation);

        AZ::Data::AssetId modelAssetId;
        AZ::Render::MeshComponentRequestBus::EventResult(modelAssetId, m_targetEntityId,
            &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);

        if (modelAssetId.IsValid())
        {
            AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset = AZ::Data::AssetManager::Instance().GetAsset(modelAssetId, azrtti_typeid<AZ::RPI::ModelAsset>(), AZ::Data::AssetLoadBehavior::PreLoad);
            modelAsset.BlockUntilLoadComplete();
            if (modelAsset.IsReady())
            {
                const AZ::Aabb& aabb = modelAsset->GetAabb();
                aabb.GetAsSphere(m_modelCenter, m_radius);

                m_distanceMin = 0.5f * AZ::GetMin(AZ::GetMin(aabb.GetExtents().GetX(), aabb.GetExtents().GetY()), aabb.GetExtents().GetZ()) + DepthNear;
                m_distanceMax = m_radius * MaxDistanceMultiplier;
            }
        }
    }

    void MaterialEditorViewportInputController::EvaluateControlBehavior()
    {
        AZStd::shared_ptr<Behavior> nextBehavior;
        auto it = m_behaviorMap.find(m_keys);
        if (it == m_behaviorMap.end())
        {
            nextBehavior = m_behaviorMap[None];
        }
        else
        {
            nextBehavior = it->second;
        }

        if (nextBehavior == m_behavior)
        {
            return;
        }

        if (m_behavior)
        {
            m_behavior->End();
        }
        m_behavior = nextBehavior;
        m_behavior->Start();
    }
} // namespace MaterialEditor

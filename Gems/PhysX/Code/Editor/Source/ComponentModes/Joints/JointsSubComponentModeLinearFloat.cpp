/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeLinearFloat.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeLinearFloat, AZ::SystemAllocator, 0);

    JointsSubComponentModeLinearFloat::JointsSubComponentModeLinearFloat(
        const AZStd::string& propertyName, float exponent, float max, float min)
        : m_propertyName(propertyName)
        , m_exponent(exponent)
        , m_inverseExponent(1.0f / exponent)
        , m_max(max)
        , m_min(min)
    {

    }

    void JointsSubComponentModeLinearFloat::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        EditorJointRequestBus::EventResult(m_resetValue, idPair, &EditorJointRequests::GetLinearValue, m_propertyName);

        const AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(idPair.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, idPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParamaterNames::Transform);

        m_manipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_manipulator->AddEntityComponentIdPair(idPair);
        m_manipulator->SetAxis(AZ::Vector3::CreateAxisX());
        m_manipulator->SetLocalTransform(localTransform);

        Refresh(idPair);

        const AZ::Color manipulatorColor(0.3f, 0.3f, 0.3f, 1.0f);
        const float manipulatorSize = 0.05f;

        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(manipulatorColor, manipulatorSize));
        m_manipulator->SetViews(AZStd::move(views));

        struct SharedState
        {
            float m_startingValue = 0.0f;
        };
        auto sharedState = AZStd::make_shared<SharedState>();

        m_manipulator->InstallLeftMouseDownCallback(
            [this, sharedState, idPair]([[maybe_unused]]const AzToolsFramework::LinearManipulator::Action& action) mutable
            {
                float currentValue = 0.0f;

                EditorJointRequestBus::EventResult(currentValue, idPair, &EditorJointRequests::GetLinearValue, m_propertyName);
                sharedState->m_startingValue = currentValue;
            });

        m_manipulator->InstallMouseMoveCallback(
            [this, sharedState, idPair](const AzToolsFramework::LinearManipulator::Action& action)
            {
                const float axisDisplacement = action.LocalPositionOffset().Dot(action.m_fixed.m_axis);

                float newValue = AZ::GetClamp(sharedState->m_startingValue + DisplacementToDeltaValue(axisDisplacement), m_min, m_max);
                EditorJointRequestBus::Event(idPair, &EditorJointRequests::SetLinearValue, m_propertyName, newValue);

                const AZ::Vector3 localPosition = action.LocalPosition().GetMax(AZ::Vector3(0.01f, 0.0f, 0.0f));
                m_manipulator->SetLocalTransform(AZ::Transform::CreateTranslation(localPosition));
            });

        m_manipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    void JointsSubComponentModeLinearFloat::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        float currentValue = 0.0f;
        EditorJointRequestBus::EventResult(currentValue, idPair, &EditorJointRequests::GetLinearValue, m_propertyName);

        m_manipulator->SetLocalTransform(AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX() * ValueToDisplacement(currentValue)));
    }

    void JointsSubComponentModeLinearFloat::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        m_manipulator->RemoveEntityComponentIdPair(idPair);
        m_manipulator->Unregister();
        m_manipulator.reset();
    }

    void JointsSubComponentModeLinearFloat::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        EditorJointRequestBus::Event(idPair, &EditorJointRequests::SetLinearValue, m_propertyName, m_resetValue);
        m_manipulator->SetLocalTransform(AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX() * ValueToDisplacement(m_resetValue)));
    }

    float JointsSubComponentModeLinearFloat::DisplacementToDeltaValue(float displacement) const
    {
        if (displacement > 0.0f)
        {
            return AZStd::pow(displacement, m_exponent);
        }
        else if (displacement < 0.0f)
        {
            return -AZStd::pow(fabsf(displacement), m_exponent);
        }
        else
        {
            return 0.0f;
        }
    }

    float JointsSubComponentModeLinearFloat::ValueToDisplacement(float value) const
    {
        return AZStd::pow(value, m_inverseExponent);
    }

} // namespace PhysX

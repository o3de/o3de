
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX_precompiled.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

#include <Editor/EditorJointComponentMode.h>
#include <Editor/EditorSubComponentModeLinear.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    EditorSubComponentModeLinear::EditorSubComponentModeLinear(
        const AZ::EntityComponentIdPair& entityComponentIdPair
        , const AZ::Uuid& componentType
        , const AZStd::string& name
        , float exponent
        , float max
        , float min)
        : EditorSubComponentModeBase(entityComponentIdPair, componentType, name)
        , m_exponent(exponent)
        , m_inverseExponent(1.0f/exponent)
        , m_max(max)
        , m_min(min)
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentId.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentId
            , &EditorJointRequests::GetTransformValue
            , PhysX::EditorJointComponentMode::s_parameterTransform);

        m_manipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_manipulator->AddEntityComponentIdPair(m_entityComponentId);
        m_manipulator->SetAxis(AZ::Vector3::CreateAxisX());
        m_manipulator->SetLocalTransform(localTransform);

        Refresh();

        const AZ::Color manipulatorColor(0.3f, 0.3f, 0.3f, 1.0f);
        const float manipulatorSize = 0.05f;

        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(manipulatorColor
            , manipulatorSize));
        m_manipulator->SetViews(AZStd::move(views));

        struct SharedState
        {
            float m_startingValue = 0.0f;
        };
        auto sharedState = AZStd::make_shared<SharedState>();

        m_manipulator->InstallLeftMouseDownCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& /*action*/) mutable
        {
            float currentValue = 0.0f;

            EditorJointRequestBus::EventResult(
                currentValue, m_entityComponentId
                , &EditorJointRequests::GetLinearValue
                , m_name);
            sharedState->m_startingValue = currentValue;
        });

        m_manipulator->InstallMouseMoveCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& action)
        {
            const float axisDisplacement = action.LocalPositionOffset().Dot(action.m_fixed.m_axis);

            float newValue = AZ::GetClamp(sharedState->m_startingValue + DisplacementToDeltaValue(axisDisplacement), m_min, m_max);
            EditorJointRequestBus::Event(
                m_entityComponentId
                , &EditorJointRequests::SetLinearValue
                , m_name
                , newValue);

            const AZ::Vector3 localPosition = action.LocalPosition().GetMax(AZ::Vector3(0.01f, 0.0f, 0.0f));
            m_manipulator->SetLocalTransform(AZ::Transform::CreateTranslation(localPosition));
            m_manipulator->SetBoundsDirty();
        });

        m_manipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorSubComponentModeLinear::~EditorSubComponentModeLinear()
    {
        m_manipulator->Unregister();
    }

    void EditorSubComponentModeLinear::Refresh()
    {
        float currentValue = 0.0f;

        EditorJointRequestBus::EventResult(
            currentValue, m_entityComponentId
            , &EditorJointRequests::GetLinearValue
            , m_name);

        m_manipulator->SetLocalTransform(AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX() * ValueToDisplacement(currentValue)));
    }

    float EditorSubComponentModeLinear::DisplacementToDeltaValue(float displacement) const
    {
        if (displacement > 0.0f)
        {
            return powf(displacement, m_exponent);
        }
        else if (displacement < 0.0f)
        {
            return -powf(fabsf(displacement), m_exponent);
        }
        else
        {
            return 0.0f;
        }
    }

    float EditorSubComponentModeLinear::ValueToDisplacement(float value) const
    {   
        return powf(value, m_inverseExponent);
    }
} // namespace PhysX

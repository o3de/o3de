/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeAngleCone.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>

#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <PhysX/EditorJointBus.h>
#include <Source/Utils.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsSubComponentModeAngleCone, AZ::SystemAllocator, 0);

    namespace Internal
    {
        const float ArrowLength = 2.0f;
        const float ConeHeight = 3.0f;
        const float XRotationManipulatorRadius = 2.0f;
        const float XRotationManipulatorWidth = 0.05f;
    } // namespace Internal

    JointsSubComponentModeAngleCone::JointsSubComponentModeAngleCone(
        const AZStd::string& propertyName, float max, float min)
        : m_propertyName(propertyName)
        , m_max(max)
        , m_min(min)
    {

    }

    void JointsSubComponentModeAngleCone::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_entityComponentIdPair = idPair;
        EditorJointRequestBus::EventResult(
            m_resetPostion, m_entityComponentIdPair, &PhysX::EditorJointRequests::GetVector3Value, JointsComponentModeCommon::ParamaterNames::Position);
        EditorJointRequestBus::EventResult(
            m_resetRotation, m_entityComponentIdPair, &PhysX::EditorJointRequests::GetVector3Value, JointsComponentModeCommon::ParamaterNames::Rotation);
        EditorJointRequestBus::EventResult(
            m_resetLimits, m_entityComponentIdPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);

        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentIdPair.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentIdPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParamaterNames::Transform);
        const AZ::Quaternion localRotation = localTransform.GetRotation();

        // Initialize manipulators used to resize the base of the cone.
        m_yLinearManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_yLinearManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_yLinearManipulator->SetAxis(AZ::Vector3::CreateAxisZ());

        m_zLinearManipulator = AzToolsFramework::LinearManipulator::MakeShared(worldTransform);
        m_zLinearManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_zLinearManipulator->SetAxis(AZ::Vector3::CreateAxisY());

        m_yzPlanarManipulator = AzToolsFramework::PlanarManipulator::MakeShared(worldTransform);
        m_yzPlanarManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_yzPlanarManipulator->SetAxes(AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());

        ConfigureLinearView(
            Internal::ArrowLength, AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), AZ::Color(0.0f, 0.0f, 1.0f, 1.0f));

        ConfigurePlanarView(AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), AZ::Color(0.0f, 0.0f, 1.0f, 1.0f));

        // Position and orientate manipulators
        AZ::Transform displacementTransform = localTransform;
        AZ::Vector3 displacementTranslate = localRotation.TransformVector(AZ::Vector3(Internal::ConeHeight, 0.0f, 0.0f));
        displacementTransform.SetTranslation(localTransform.GetTranslation() + displacementTranslate);

        m_yLinearManipulator->SetLocalTransform(displacementTransform);
        m_zLinearManipulator->SetLocalTransform(displacementTransform);
        m_yzPlanarManipulator->SetLocalTransform(displacementTransform);

        // Initialize rotation manipulator for rotating cone
        m_xRotationManipulator = AzToolsFramework::AngularManipulator::MakeShared(worldTransform);
        m_xRotationManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_xRotationManipulator->SetAxis(AZ::Vector3::CreateAxisX());
        m_xRotationManipulator->SetLocalTransform(localTransform);

        const AZ::Color xRotationManipulatorColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        m_xRotationManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_xRotationManipulator, xRotationManipulatorColor, Internal::XRotationManipulatorRadius, Internal::XRotationManipulatorWidth,
            AzToolsFramework::DrawHalfDottedCircle));

        AZStd::shared_ptr<JointsComponentModeCommon::SubComponentModes::AngleModesSharedRotationState> sharedRotationState =
            AZStd::make_shared<JointsComponentModeCommon::SubComponentModes::AngleModesSharedRotationState>();

        struct SharedState
        {
            AngleLimitsFloatPair m_startValues;
        };
        auto sharedState = AZStd::make_shared<SharedState>();

        m_yLinearManipulator->InstallLeftMouseDownCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& /*action*/) mutable
            {
                AngleLimitsFloatPair currentValue;
                EditorJointRequestBus::EventResult(
                    currentValue, m_entityComponentIdPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);
                sharedState->m_startValues = currentValue;
            });

        m_yLinearManipulator->InstallMouseMoveCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& action)
            {
                AZ::Transform localTransform = AZ::Transform::CreateIdentity();
                EditorJointRequestBus::EventResult(
                    localTransform, m_entityComponentIdPair, &EditorJointRequests::GetTransformValue,
                    JointsComponentModeCommon::ParamaterNames::Transform);
                const AZ::Quaternion localRotation = localTransform.GetRotation();
                const float axisDisplacement = action.LocalPositionOffset().Dot(localRotation.TransformVector(action.m_fixed.m_axis));
                const float originalBaseY = tan(AZ::DegToRad(sharedState->m_startValues.first)) * Internal::ConeHeight;
                const float newBaseY = originalBaseY + axisDisplacement;
                const float newAngle = AZ::GetClamp(AZ::RadToDeg(atan(newBaseY / Internal::ConeHeight)), m_min, m_max);

                EditorJointRequestBus::Event(
                    m_entityComponentIdPair, &EditorJointRequests::SetLinearValuePair, m_propertyName,
                    AngleLimitsFloatPair(newAngle, sharedState->m_startValues.second));

                m_yLinearManipulator->SetBoundsDirty();
            });

        m_zLinearManipulator->InstallLeftMouseDownCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& /*action*/) mutable
            {
                AngleLimitsFloatPair currentValue;
                EditorJointRequestBus::EventResult(
                    currentValue, m_entityComponentIdPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);
                sharedState->m_startValues = currentValue;
            });

        m_zLinearManipulator->InstallMouseMoveCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& action)
            {
                AZ::Transform localTransform = AZ::Transform::CreateIdentity();
                EditorJointRequestBus::EventResult(
                    localTransform, m_entityComponentIdPair, &EditorJointRequests::GetTransformValue,
                    JointsComponentModeCommon::ParamaterNames::Transform);
                const AZ::Quaternion localRotation = localTransform.GetRotation();
                const float axisDisplacement = action.LocalPositionOffset().Dot(localRotation.TransformVector(action.m_fixed.m_axis));
                const float originalBaseZ = tan(AZ::DegToRad(sharedState->m_startValues.second)) * Internal::ConeHeight;
                const float newBaseZ = originalBaseZ + axisDisplacement;
                const float newAngle = AZ::GetClamp(AZ::RadToDeg(atan(newBaseZ / Internal::ConeHeight)), m_min, m_max);

                EditorJointRequestBus::Event(
                    m_entityComponentIdPair, &EditorJointRequests::SetLinearValuePair, m_propertyName,
                    AngleLimitsFloatPair(sharedState->m_startValues.first, newAngle));

                m_zLinearManipulator->SetBoundsDirty();
            });

        m_yzPlanarManipulator->InstallLeftMouseDownCallback(
            [this, sharedState]([[maybe_unused]]const AzToolsFramework::PlanarManipulator::Action& action) mutable
            {
                AngleLimitsFloatPair currentValue;
                EditorJointRequestBus::EventResult(
                    currentValue, m_entityComponentIdPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);
                sharedState->m_startValues = currentValue;
            });

        m_yzPlanarManipulator->InstallMouseMoveCallback(
            [this, sharedState](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                AZ::Transform localTransform = AZ::Transform::CreateIdentity();
                EditorJointRequestBus::EventResult(
                    localTransform, m_entityComponentIdPair, &EditorJointRequests::GetTransformValue,
                    JointsComponentModeCommon::ParamaterNames::Transform);

                const AZ::Quaternion localRotation = localTransform.GetRotation();

                const float axisDisplacementY = action.LocalPositionOffset().Dot(localRotation.TransformVector(AZ::Vector3::CreateAxisY()));
                const float axisDisplacementZ = action.LocalPositionOffset().Dot(localRotation.TransformVector(AZ::Vector3::CreateAxisZ()));
                const float axisDisplacement = axisDisplacementZ > axisDisplacementY ? axisDisplacementZ : axisDisplacementY;

                const float originalBaseY = tan(AZ::DegToRad(sharedState->m_startValues.first)) * Internal::ConeHeight;
                const float newBaseY = originalBaseY + axisDisplacement;
                const float newAngleY = AZ::GetClamp(AZ::RadToDeg(atan(newBaseY / Internal::ConeHeight)), m_min, m_max);

                const float originalBaseZ = tan(AZ::DegToRad(sharedState->m_startValues.second)) * Internal::ConeHeight;
                const float newBaseZ = originalBaseZ + axisDisplacement;
                const float newAngleZ = AZ::GetClamp(AZ::RadToDeg(atan(newBaseZ / Internal::ConeHeight)), m_min, m_max);

                EditorJointRequestBus::Event(
                    m_entityComponentIdPair, &EditorJointRequests::SetLinearValuePair, m_propertyName,
                    AngleLimitsFloatPair(newAngleY, newAngleZ));

                m_yzPlanarManipulator->SetBoundsDirty();
            });

        struct SharedStateXRotate
        {
            AZ::Transform m_startTM;
        };
        auto sharedStateXRotate = AZStd::make_shared<SharedStateXRotate>();

        auto mouseDownCallback = [this, sharedRotationState](const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            AZ::Quaternion normalizedStart = action.m_start.m_rotation.GetNormalized();
            sharedRotationState->m_axis = AZ::Vector3(normalizedStart.GetX(), normalizedStart.GetY(), normalizedStart.GetZ());
            sharedRotationState->m_savedOrientation = AZ::Quaternion::CreateIdentity();

            AngleLimitsFloatPair currentValue;
            EditorJointRequestBus::EventResult(
                currentValue, m_entityComponentIdPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);

            sharedRotationState->m_valuePair = currentValue;
        };

        auto mouseDownRotateXCallback =
            [this, sharedStateXRotate]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
        {
            PhysX::EditorJointRequestBus::EventResult(
                sharedStateXRotate->m_startTM, m_entityComponentIdPair, &PhysX::EditorJointRequests::GetTransformValue,
                JointsComponentModeCommon::ParamaterNames::Transform);
        };

        m_xRotationManipulator->InstallLeftMouseDownCallback(mouseDownRotateXCallback);

        m_xRotationManipulator->InstallMouseMoveCallback(
            [this, sharedStateXRotate](const AzToolsFramework::AngularManipulator::Action& action) mutable -> void
            {
                const AZ::Quaternion manipulatorOrientation = action.m_start.m_rotation * action.m_current.m_delta;

                AZ::Transform newTransform = AZ::Transform::CreateIdentity();
                newTransform = sharedStateXRotate->m_startTM * AZ::Transform::CreateFromQuaternion(action.m_current.m_delta);

                PhysX::EditorJointRequestBus::Event(
                    m_entityComponentIdPair, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Position,
                    newTransform.GetTranslation());
                PhysX::EditorJointRequestBus::Event(
                    m_entityComponentIdPair, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Rotation,
                    newTransform.GetRotation().GetEulerDegrees());

                m_yLinearManipulator->SetLocalOrientation(manipulatorOrientation);
                m_zLinearManipulator->SetLocalOrientation(manipulatorOrientation);
                m_yLinearManipulator->SetAxis(action.m_current.m_delta.TransformVector(AZ::Vector3::CreateAxisY()));
                m_zLinearManipulator->SetAxis(action.m_current.m_delta.TransformVector(AZ::Vector3::CreateAxisZ()));
                m_xRotationManipulator->SetLocalOrientation(manipulatorOrientation);

                m_yLinearManipulator->SetBoundsDirty();
                m_zLinearManipulator->SetBoundsDirty();
                m_xRotationManipulator->SetBoundsDirty();
            });

        m_xRotationManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_yLinearManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_zLinearManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
        m_yzPlanarManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());

        Refresh(m_entityComponentIdPair);
    }

    void JointsSubComponentModeAngleCone::Refresh(const AZ::EntityComponentIdPair& idPair)
    {
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, idPair, &EditorJointRequests::GetTransformValue, JointsComponentModeCommon::ParamaterNames::Transform);

        float coneHeight = Internal::ConeHeight;
        AngleLimitsFloatPair yzSwingAngleLimits;
        EditorJointRequestBus::EventResult(yzSwingAngleLimits, idPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);

        // Draw inverted cone (negative cone height) if angles are larger than 90 deg.
        if (yzSwingAngleLimits.first > 90.0f || yzSwingAngleLimits.second > 90.0f)
        {
            coneHeight = -Internal::ConeHeight;
        }

        // reposition manipulators
        const AZ::Quaternion localRotation = localTransform.GetRotation();
        const AZ::Vector3 linearManipulatorOffset =
            localTransform.GetTranslation() + localRotation.TransformVector(AZ::Vector3(coneHeight, 0.0f, 0.0f));

        m_xRotationManipulator->SetLocalTransform(localTransform);

        localTransform.SetTranslation(linearManipulatorOffset);

        m_yLinearManipulator->SetLocalTransform(localTransform);
        m_zLinearManipulator->SetLocalTransform(localTransform);
        m_yzPlanarManipulator->SetLocalTransform(localTransform);
    }

    void JointsSubComponentModeAngleCone::Teardown(const AZ::EntityComponentIdPair& idPair)
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();

        m_xRotationManipulator->RemoveEntityComponentIdPair(idPair);
        m_xRotationManipulator->Unregister();
        m_yLinearManipulator->RemoveEntityComponentIdPair(idPair);
        m_yLinearManipulator->Unregister();
        m_zLinearManipulator->RemoveEntityComponentIdPair(idPair);
        m_zLinearManipulator->Unregister();
        m_yzPlanarManipulator->RemoveEntityComponentIdPair(idPair);
        m_yzPlanarManipulator->Unregister();
    }

    void JointsSubComponentModeAngleCone::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        EditorJointRequestBus::Event(
            idPair, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Position, m_resetPostion);
        EditorJointRequestBus::Event(
            idPair, &PhysX::EditorJointRequests::SetVector3Value, JointsComponentModeCommon::ParamaterNames::Rotation, m_resetRotation);
        EditorJointRequestBus::Event(idPair, &EditorJointRequests::SetLinearValuePair, m_propertyName, m_resetLimits);
    }

    void JointsSubComponentModeAngleCone::ConfigureLinearView(
        float axisLength, [[maybe_unused]] const AZ::Color& axis1Color, const AZ::Color& axis2Color, const AZ::Color& axis3Color)
    {
        const float coneLength = 0.28f;
        const float coneRadius = 0.07f;

        const auto configureLinearView =
            [coneLength, axisLength, coneRadius](AzToolsFramework::LinearManipulator* linearManipulator, const AZ::Color& color)
        {
            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(CreateManipulatorViewLine(
                *linearManipulator, color, axisLength,
                AzToolsFramework::ManipulatorLineBoundWidth(AzFramework::InvalidViewportId)));
            views.emplace_back(CreateManipulatorViewCone(
                *linearManipulator, color, linearManipulator->GetAxis() * (axisLength - coneLength), coneLength, coneRadius));
            linearManipulator->SetViews(AZStd::move(views));
        };

        configureLinearView(m_yLinearManipulator.get(), axis2Color);
        configureLinearView(m_zLinearManipulator.get(), axis3Color);
    }

    void JointsSubComponentModeAngleCone::ConfigurePlanarView(const AZ::Color& planeColor, const AZ::Color& plane2Color)
    {
        const float planeSize = 0.6f;
        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(CreateManipulatorViewQuad(*m_yzPlanarManipulator, planeColor, plane2Color, planeSize));
        m_yzPlanarManipulator->SetViews(AZStd::move(views));
    }

    void JointsSubComponentModeAngleCone::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ::Transform worldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(m_entityComponentIdPair.GetEntityId());

        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EditorJointRequestBus::EventResult(
            localTransform, m_entityComponentIdPair, &EditorJointRequests::GetTransformValue,
            JointsComponentModeCommon::ParamaterNames::Transform);

        AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();

        debugDisplay.PushMatrix(worldTransform);
        debugDisplay.PushMatrix(localTransform);

        const float xAxisArrowLength = 2.0f;
        debugDisplay.SetColor(AZ::Color(1.0f, 0.0f, 0.0f, 1.0f));
        debugDisplay.DrawArrow(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(xAxisArrowLength, 0.0f, 0.0f));

        AngleLimitsFloatPair yzSwingAngleLimits;
        EditorJointRequestBus::EventResult(
            yzSwingAngleLimits, m_entityComponentIdPair, &EditorJointRequests::GetLinearValuePair, m_propertyName);

        const AZ::u32 numEllipseSamples = 16;
        AZ::Vector3 ellipseSamples[numEllipseSamples];
        float coneHeight = Internal::ConeHeight;

        // Draw inverted cone if angles are larger than 90 deg.
        if (yzSwingAngleLimits.first > 90.0f || yzSwingAngleLimits.second > 90.0f)
        {
            coneHeight = -Internal::ConeHeight;
        }

        // Compute points along perimeter of cone base
        const float coney = tanf(AZ::DegToRad(yzSwingAngleLimits.first)) * coneHeight;
        const float conez = tanf(AZ::DegToRad(yzSwingAngleLimits.second)) * coneHeight;
        const float step = AZ::Constants::TwoPi / numEllipseSamples;
        for (size_t i = 0; i < numEllipseSamples; ++i)
        {
            const float angleStep = step * i;
            ellipseSamples[i].SetX(coneHeight);
            ellipseSamples[i].SetY(conez * sin(angleStep));
            ellipseSamples[i].SetZ(coney * cos(angleStep));
        }

        // draw cone
        for (size_t i = 0; i < numEllipseSamples; ++i)
        {
            size_t nextIndex = i + 1;
            if (i == numEllipseSamples - 1)
            {
                nextIndex = 0;
            }

            // draw cone sides
            debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, 0.2f));
            debugDisplay.DrawTri(AZ::Vector3(0.0f, 0.0f, 0.0f), ellipseSamples[i], ellipseSamples[nextIndex]);

            // draw parameter of cone base
            debugDisplay.SetColor(AZ::Color(0.4f, 0.4f, 0.4f, 0.4f));
            debugDisplay.DrawLine(ellipseSamples[i], ellipseSamples[nextIndex]);
        }

        // draw axis lines at base of cone, and from tip to base.
        debugDisplay.SetColor(AZ::Color(0.5f, 0.5f, 0.5f, 0.6f));
        debugDisplay.DrawLine(ellipseSamples[0], ellipseSamples[numEllipseSamples / 2]);
        debugDisplay.DrawLine(ellipseSamples[numEllipseSamples * 3 / 4], ellipseSamples[numEllipseSamples / 4]);
        debugDisplay.DrawLine(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(coneHeight, 0.0f, 0.0f));

        debugDisplay.PopMatrix(); // pop local transform
        debugDisplay.PopMatrix(); // pop world transform
        debugDisplay.SetState(stateBefore);

        // reposition and reorientate manipulators
        Refresh(m_entityComponentIdPair);
    }

} // namespace PhysX

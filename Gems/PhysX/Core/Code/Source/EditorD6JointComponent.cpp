/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentModeCommon.h>
#include <Source/D6JointComponent.h>
#include <Source/EditorD6JointComponent.h>
#include <Source/Utils.h>

namespace PhysX
{
    namespace
    {
        const float JointDisplayAlpha = 0.6f;
        const AZ::Color ColorLinearAxis(1.0f, 1.0f, 1.0f, JointDisplayAlpha);
        const AZ::Color ColorTwistArc(1.0f, 1.0f, 1.0f, JointDisplayAlpha);
        const AZ::Color ColorTwistLower(1.0f, 0.0f, 0.0f, JointDisplayAlpha);
        const AZ::Color ColorTwistUpper(0.0f, 1.0f, 0.0f, JointDisplayAlpha);
        const AZ::Color ColorSwingCone(1.0f, 1.0f, 1.0f, 0.5f);
        const AZ::Color ColorSwingFree(1.0f, 1.0f, 1.0f, JointDisplayAlpha);

        //! A free axis gets a double headed arrow, a limited one a bar spanning its limits with ball end caps.
        void DrawLinearAxis(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::Vector3& axis,
            D6JointAxis motion,
            const AZStd::pair<float, float>& limits,
            float scaleMultiply)
        {
            if (motion == D6JointAxis::Locked)
            {
                return;
            }

            debugDisplay.SetColor(ColorLinearAxis);

            if (motion == D6JointAxis::Free)
            {
                const AZ::Vector3 extent = axis * 4.0f * scaleMultiply;
                debugDisplay.DrawArrow(-extent, extent, scaleMultiply, true);
                return;
            }

            // linear limits are authored in metres, so the bar is drawn at world scale rather than screen scale
            const AZ::Vector3 lower = axis * AZ::GetMin(limits.first, limits.second);
            const AZ::Vector3 upper = axis * AZ::GetMax(limits.first, limits.second);
            const float capRadius = 0.05f * scaleMultiply;
            debugDisplay.DrawLine(lower, upper);
            debugDisplay.DrawBall(lower, capRadius, false);
            debugDisplay.DrawBall(upper, capRadius, false);
        }

        //! Twist is rotation about the joint X axis: an arc over the swept range, with a fin at each limit.
        void DrawTwist(
            AzFramework::DebugDisplayRequests& debugDisplay,
            D6JointAxis motion,
            const AZStd::pair<float, float>& limitsDegrees,
            float scaleMultiply)
        {
            if (motion == D6JointAxis::Locked)
            {
                return;
            }

            const AZ::Vector3 axis = AZ::Vector3::CreateAxisX();
            const float size = 2.0f * scaleMultiply;
            const float arcRadius = 1.0f * scaleMultiply;
            const float arcGranularity = 1.0f;

            debugDisplay.SetColor(ColorTwistArc);
            if (motion == D6JointAxis::Free)
            {
                debugDisplay.DrawArc(AZ::Vector3::CreateZero(), arcRadius, 0.0f, 360.0f, arcGranularity, -axis);
                return;
            }

            const float lower = AZ::GetMin(limitsDegrees.first, limitsDegrees.second);
            const float upper = AZ::GetMax(limitsDegrees.first, limitsDegrees.second);
            debugDisplay.DrawArc(AZ::Vector3::CreateZero(), arcRadius, lower, upper - lower, arcGranularity, -axis);

            const AZ::Vector3 axisPoint = axis * size * 0.5f;
            AZ::Vector3 points[4] = { -axisPoint, axisPoint, axisPoint, -axisPoint };
            points[2].SetZ(size);
            points[3].SetZ(size);

            const AZ::Color finColors[2] = { ColorTwistLower, ColorTwistUpper };
            const float finAngles[2] = { lower, upper };
            for (size_t i = 0; i < 2; ++i)
            {
                const AZ::Transform finTM =
                    AZ::Transform::CreateFromQuaternion(AZ::Quaternion::CreateFromAxisAngle(axis, AZ::DegToRad(finAngles[i])));
                debugDisplay.PushMatrix(finTM);
                debugDisplay.SetColor(finColors[i]);
                debugDisplay.DrawQuad(points[0], points[1], points[2], points[3]);
                debugDisplay.PopMatrix();
            }
        }

        //! Sweep of the joint X axis about rotationAxis, between -halfAngle and +halfAngle.
        void DrawSwingSweep(
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::Vector3& rotationAxis,
            float halfAngleDegrees,
            bool filled,
            float scaleMultiply)
        {
            constexpr AZ::u32 NumSweepSamples = 24;
            const AZ::Vector3 start = AZ::Vector3::CreateAxisX() * 3.0f * scaleMultiply;

            AZ::Vector3 samples[NumSweepSamples + 1];
            for (AZ::u32 i = 0; i <= NumSweepSamples; ++i)
            {
                const float angle = -halfAngleDegrees + 2.0f * halfAngleDegrees * i / NumSweepSamples;
                samples[i] = AZ::Quaternion::CreateFromAxisAngle(rotationAxis, AZ::DegToRad(angle)).TransformVector(start);
            }

            for (AZ::u32 i = 0; i < NumSweepSamples; ++i)
            {
                if (filled)
                {
                    debugDisplay.DrawTri(AZ::Vector3::CreateZero(), samples[i], samples[i + 1]);
                }
                else
                {
                    debugDisplay.DrawLine(samples[i], samples[i + 1]);
                }
            }

            if (filled)
            {
                debugDisplay.DrawLine(AZ::Vector3::CreateZero(), samples[0]);
                debugDisplay.DrawLine(AZ::Vector3::CreateZero(), samples[NumSweepSamples]);
            }
        }

        //! Elliptical cone about the joint X axis, matching the single PxJointLimitCone both swing axes share.
        void DrawSwingCone(
            AzFramework::DebugDisplayRequests& debugDisplay, const D6JointComponentConfiguration& config, float scaleMultiply)
        {
            // an inverted cone reads better once a half-angle passes 90 degrees
            const bool inverted = config.m_motionLimitsCone.first > 90.0f || config.m_motionLimitsCone.second > 90.0f;
            const float coneHeight = (inverted ? -3.0f : 3.0f) * scaleMultiply;
            const float coneY = tan(AZ::DegToRad(config.m_motionLimitsCone.first)) * coneHeight;
            const float coneZ = tan(AZ::DegToRad(config.m_motionLimitsCone.second)) * coneHeight;

            constexpr AZ::u32 NumEllipseSamples = 16;
            AZ::Vector3 ellipseSamples[NumEllipseSamples];
            const float step = AZ::Constants::TwoPi / NumEllipseSamples;
            for (AZ::u32 i = 0; i < NumEllipseSamples; ++i)
            {
                const float angleStep = step * i;
                ellipseSamples[i].SetX(coneHeight);
                ellipseSamples[i].SetY(coneZ * sin(angleStep));
                ellipseSamples[i].SetZ(coneY * cos(angleStep));
            }

            debugDisplay.SetColor(ColorSwingCone);
            for (AZ::u32 i = 0; i < NumEllipseSamples; ++i)
            {
                const AZ::u32 nextIndex = (i + 1) % NumEllipseSamples;
                debugDisplay.DrawTri(AZ::Vector3::CreateZero(), ellipseSamples[i], ellipseSamples[nextIndex]);
            }
        }

        //! Swing 1 rotates about Y, swing 2 about Z. Only a pair of limited axes forms a cone.
        void DrawSwing(AzFramework::DebugDisplayRequests& debugDisplay, const D6JointComponentConfiguration& config, float scaleMultiply)
        {
            const AZ::Vector3 swing1Axis = AZ::Vector3::CreateAxisY();
            const AZ::Vector3 swing2Axis = AZ::Vector3::CreateAxisZ();

            debugDisplay.SetColor(ColorSwingCone);
            if (config.IsSwing1Limited() && config.IsSwing2Limited())
            {
                DrawSwingCone(debugDisplay, config, scaleMultiply);
            }
            else if (config.IsSwing1Limited())
            {
                DrawSwingSweep(debugDisplay, swing1Axis, config.m_motionLimitsCone.first, true, scaleMultiply);
            }
            else if (config.IsSwing2Limited())
            {
                DrawSwingSweep(debugDisplay, swing2Axis, config.m_motionLimitsCone.second, true, scaleMultiply);
            }

            debugDisplay.SetColor(ColorSwingFree);
            if (config.m_motionSwing1 == D6JointAxis::Free)
            {
                DrawSwingSweep(debugDisplay, swing1Axis, 180.0f, false, scaleMultiply);
            }
            if (config.m_motionSwing2 == D6JointAxis::Free)
            {
                DrawSwingSweep(debugDisplay, swing2Axis, 180.0f, false, scaleMultiply);
            }
        }
    } // namespace

    void EditorD6JointComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorD6JointComponent, EditorJointComponent>()
                ->Version(1)
                ->Field("D6 Limits", &EditorD6JointComponent::m_d6Config)
                ->Field("Component Mode", &EditorD6JointComponent::m_componentModeDelegate);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                AZStd::vector<AZ::Crc32> componentMenus;
                if (D6JointsEnabled())
                {
                    componentMenus.emplace_back(AZ::Crc32("Game"));
                }

                editContext->Class<EditorD6JointComponent>("PhysX D6 Joint", "D6 joint provides 6 degree of freedom constraint")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysicsConstraint.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysicsConstraint.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, componentMenus)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorD6JointComponent::m_d6Config, "D6 Limits", "D6 joint limits");
            }
        }
    }

    void EditorD6JointComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsJointService"));
    }

    void EditorD6JointComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("PhysicsDynamicRigidBodyService"));
    }

    void EditorD6JointComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorD6JointComponent::Activate()
    {
        EditorJointComponent::Activate();
    }

    void EditorD6JointComponent::Deactivate()
    {
        EditorJointComponent::Deactivate();
    }

    void EditorD6JointComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        m_config.m_followerEntity = GetEntityId();

        gameEntity->CreateComponent<D6JointComponent>(m_config.ToGameTimeConfig(), m_config.ToGenericProperties(), m_d6Config);
    }

    float EditorD6JointComponent::GetLinearValue([[maybe_unused]] const AZStd::string& parameterName)
    {
        AZ_Warning("EditorD6JointComponent", false, "Not implemented for Editor component");
        return 0.0f;
    }

    AngleLimitsFloatPair EditorD6JointComponent::GetLinearValuePair([[maybe_unused]] const AZStd::string& parameterName)
    {
        AZ_Warning("EditorD6JointComponent", false, "Not implemented for Editor component");
        return AngleLimitsFloatPair();
    }

    AZStd::vector<JointsComponentModeCommon::SubModeParameterState> EditorD6JointComponent::GetSubComponentModesState()
    {
        return AZStd::vector<JointsComponentModeCommon::SubModeParameterState>();
    }

    void EditorD6JointComponent::SetBoolValue([[maybe_unused]] const AZStd::string& parameterName, [[maybe_unused]] bool value)
    {
        AZ_Warning("EditorD6JointComponent", false, "Not implemented for Editor component");
    }

    void EditorD6JointComponent::SetLinearValue([[maybe_unused]] const AZStd::string& parameterName, [[maybe_unused]] float value)
    {
        AZ_Warning("EditorD6JointComponent", false, "Not implemented for Editor component");
    }

    void EditorD6JointComponent::SetLinearValuePair(
        [[maybe_unused]] const AZStd::string& parameterName, [[maybe_unused]] const AngleLimitsFloatPair& valuePair)
    {
        AZ_Warning("EditorD6JointComponent", false, "Not implemented for Editor component");
    }

    void EditorD6JointComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        EditorJointComponent::DisplayEntityViewport(viewportInfo, debugDisplay);

        if (!m_config.ShowSetupDisplay() && !m_config.m_inComponentMode)
        {
            return;
        }

        const AZ::Transform jointWorldTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(GetEntityId()) *
            GetTransformValue(PhysX::JointsComponentModeCommon::ParameterNames::Transform);
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        // scaleMultiply keeps the debug draw the same size on screen regardless of camera distance
        const float scaleMultiply = AzToolsFramework::CalculateScreenToWorldMultiplier(jointWorldTransform.GetTranslation(), cameraState);

        const AZ::u32 stateBefore = debugDisplay.GetState();
        debugDisplay.CullOff();
        debugDisplay.SetAlpha(JointDisplayAlpha);
        debugDisplay.PushMatrix(jointWorldTransform);

        DrawLinearAxis(debugDisplay, AZ::Vector3::CreateAxisX(), m_d6Config.m_motionX, m_d6Config.m_motionLimitsX, scaleMultiply);
        DrawLinearAxis(debugDisplay, AZ::Vector3::CreateAxisY(), m_d6Config.m_motionY, m_d6Config.m_motionLimitsY, scaleMultiply);
        DrawLinearAxis(debugDisplay, AZ::Vector3::CreateAxisZ(), m_d6Config.m_motionZ, m_d6Config.m_motionLimitsZ, scaleMultiply);

        DrawTwist(debugDisplay, m_d6Config.m_motionTwist, m_d6Config.m_motionLimitsTwist, scaleMultiply);
        DrawSwing(debugDisplay, m_d6Config, scaleMultiply);

        debugDisplay.PopMatrix(); // pop joint world transform
        debugDisplay.SetState(stateBefore);
    }
} // namespace PhysX

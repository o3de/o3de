/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponentMode.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void NonUniformScaleComponentMode::Reflect(AZ::ReflectContext* context)
        {
            AzToolsFramework::ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<NonUniformScaleComponentMode>(context);
        }

        NonUniformScaleComponentMode::NonUniformScaleComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
            : EditorBaseComponentMode(entityComponentIdPair, componentType)
        {
            m_entityComponentIdPair = entityComponentIdPair;

            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            worldFromLocal.ExtractUniformScale();
            m_manipulators = AZStd::make_unique<ScaleManipulators>(worldFromLocal);
            m_manipulators->Register(g_mainManipulatorManagerId);
            m_manipulators->AddEntityComponentIdPair(entityComponentIdPair);
            m_manipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
            const float axisLength = 2.0f;
            m_manipulators->ConfigureView(
                axisLength, AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
                AzFramework::ViewportColors::ZAxisColor);

            auto mouseDownCallback = [this]([[maybe_unused]] const LinearManipulator::Action& action)
            {
                AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
                AZ::NonUniformScaleRequestBus::EventResult(
                    nonUniformScale, m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

                m_initialScale = nonUniformScale;

                AZ::NonUniformScaleRequestBus::Event(
                    m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::SetScale, m_initialScale);
            };

            m_manipulators->InstallAxisLeftMouseDownCallback(mouseDownCallback);

            m_manipulators->InstallAxisMouseMoveCallback(
                [this](const LinearManipulator::Action& action)
                {
                    const AZ::Vector3 scaleMultiplier =
                        (AZ::Vector3::CreateOne() + ((action.LocalScaleOffset() * action.m_start.m_sign) / m_initialScale));

                    AZ::NonUniformScaleRequestBus::Event(
                        m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::SetScale,
                        (scaleMultiplier * m_initialScale)
                            .GetClamp(AZ::Vector3(AZ::MinTransformScale), AZ::Vector3(AZ::MaxTransformScale)));
                });

            m_manipulators->InstallUniformLeftMouseDownCallback(mouseDownCallback);

            m_manipulators->InstallUniformMouseMoveCallback(
                [this](const LinearManipulator::Action& action)
                {
                    const auto sumVectorElements = [](const AZ::Vector3& vec)
                    {
                        return vec.GetX() + vec.GetY() + vec.GetZ();
                    };

                    const float minScaleMultiplier = AZ::MinTransformScale / m_initialScale.GetMinElement();
                    const float maxScaleMultiplier = AZ::MaxTransformScale / m_initialScale.GetMaxElement();
                    const float scaleMultiplier = AZ::GetClamp(
                        1.0f + sumVectorElements(action.m_start.m_sign * action.LocalScaleOffset() / m_initialScale), minScaleMultiplier,
                        maxScaleMultiplier);

                    AZ::NonUniformScaleRequestBus::Event(
                        m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::SetScale, scaleMultiplier * m_initialScale);
                });
        }

        NonUniformScaleComponentMode::~NonUniformScaleComponentMode()
        {
            if (m_manipulators)
            {
                m_manipulators->Unregister();
            }
            m_manipulators.reset();
        }

        void NonUniformScaleComponentMode::Refresh()
        {
        }

        AZStd::string NonUniformScaleComponentMode::GetComponentModeName() const
        {
            return "Non Uniform Scale Edit Mode";
        }

        AZ::Uuid NonUniformScaleComponentMode::GetComponentModeType() const
        {
            return azrtti_typeid<NonUniformScaleComponentMode>();
        }
    } // namespace Components
} // namespace AzToolsFramework

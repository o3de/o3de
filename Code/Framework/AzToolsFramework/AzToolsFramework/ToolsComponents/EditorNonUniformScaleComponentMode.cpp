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

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponentMode.h>

namespace AzToolsFramework
{
    namespace Components
    {
        NonUniformScaleComponentMode::NonUniformScaleComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
            : EditorBaseComponentMode(entityComponentIdPair, componentType)
        {
            m_entityComponentIdPair = entityComponentIdPair;

            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            worldFromLocal.ExtractScale();

            m_manipulators = AZStd::make_unique<ScaleManipulators>(worldFromLocal);

            m_manipulators->Register(g_mainManipulatorManagerId);

            m_manipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());

            m_manipulators->ConfigureView(
                2.0f, AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
                AzFramework::ViewportColors::ZAxisColor);

            m_manipulators->InstallAxisLeftMouseDownCallback([this](const LinearManipulator::Action& action) {
                AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();

                AZ::NonUniformScaleRequestBus::EventResult(
                    nonUniformScale, m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

                m_initialScale = nonUniformScale + action.m_start.m_scaleSnapOffset;

                AZ::NonUniformScaleRequestBus::Event(
                    m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::SetScale, m_initialScale);
            });

            m_manipulators->InstallAxisMouseMoveCallback([this](const LinearManipulator::Action& action) {
                const AZ::Vector3 scale =
                    (AZ::Vector3::CreateOne() + ((action.LocalScaleOffset() * action.m_start.m_sign) / m_initialScale))
                        .GetMax(AZ::Vector3(AZ::MinTransformScale));

                AZ::NonUniformScaleRequestBus::Event(
                    m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequests::SetScale, scale * m_initialScale);
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
    } // namespace Components
} // namespace AzToolsFramework

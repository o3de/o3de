/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ShapeOffsetManipulatorRequestBus.h>

namespace AzToolsFramework
{
    void ShapeTranslationOffsetViewportEdit::Setup(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequestBus::Events::GetScale);

        AZ::Vector3 translationOffset = AZ::Vector3::CreateZero();
        ShapeOffsetManipulatorRequestBus::EventResult(
            translationOffset, entityComponentIdPair, &ShapeOffsetManipulatorRequestBus::Events::GetTranslationOffset);

        m_translationManipulators = AZStd::make_shared<AzToolsFramework::TranslationManipulators>(
        AzToolsFramework::TranslationManipulators::Dimensions::Three, worldTransform, nonUniformScale);

        m_translationManipulators->SetLocalTransform(AZ::Transform::CreateTranslation(translationOffset));
        m_translationManipulators->SetLineBoundWidth(AzToolsFramework::ManipulatorLineBoundWidth());
        m_translationManipulators->AddEntityComponentIdPair(m_entityComponentIdPair);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(m_translationManipulators.get());

        auto mouseMoveHandlerFn = [this,
                                   transformScale{ m_translationManipulators->GetSpace().GetUniformScale() }
                                   /*nonUniformScale{m_translationManipulators->GetNonUniformScale()}*/](const auto& action)
        {
            AZ::Vector3 translationOffset = AZ::Vector3::CreateZero();
            ShapeOffsetManipulatorRequestBus::EventResult(
                translationOffset, m_entityComponentIdPair, &ShapeOffsetManipulatorRequestBus::Events::GetTranslationOffset);

            const AZ::Transform manipulatorLocalTransform = AZ::Transform::CreateTranslation(translationOffset);
            const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(transformScale, manipulatorLocalTransform, action);

            ShapeOffsetManipulatorRequestBus::Event(
                m_entityComponentIdPair,
                &ShapeOffsetManipulatorRequestBus::Events::SetTranslationOffset,
                translationOffset + manipulatorPosition);

            UpdateManipulators();
        };

        m_translationManipulators->InstallLinearManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        m_translationManipulators->InstallPlanarManipulatorMouseMoveCallback(mouseMoveHandlerFn);

        m_translationManipulators->Register(g_mainManipulatorManagerId);
    }

    void ShapeTranslationOffsetViewportEdit::Teardown()
    {
        m_translationManipulators->Unregister();
        m_translationManipulators.reset();
    }

    void ShapeTranslationOffsetViewportEdit::UpdateManipulators()
    {
        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, m_entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequestBus::Events::GetScale);

        AZ::Vector3 translationOffset = AZ::Vector3::CreateZero();
        ShapeOffsetManipulatorRequestBus::EventResult(
            translationOffset, m_entityComponentIdPair, &ShapeOffsetManipulatorRequestBus::Events::GetTranslationOffset);

        m_translationManipulators->SetSpace(worldTransform);
        m_translationManipulators->SetLocalTransform(AZ::Transform::CreateTranslation(translationOffset));
        m_translationManipulators->SetNonUniformScale(nonUniformScale);
        m_translationManipulators->SetBoundsDirty();
    }
} // namespace AzToolsFramework

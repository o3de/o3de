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
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>

namespace AzToolsFramework
{
    void ShapeTranslationOffsetViewportEdit::AddEntityComponentIdPairImpl(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        if (m_translationManipulators)
        {
            m_translationManipulators->AddEntityComponentIdPair(entityComponentIdPair);
        }
        else
        {
            AZ_WarningOnce(
                "ShapeTranslationOffsetViewportEdit",
                false,
                "Attempting to AddEntityComponentIdPair before manipulators have been created");
        }
    }

    void ShapeTranslationOffsetViewportEdit::Setup(const ManipulatorManagerId manipulatorManagerId)
    {
        const AZ::Transform manipulatorSpace = GetManipulatorSpace();
        const AZ::Vector3 nonUniformScale = GetNonUniformScale();
        const AZ::Vector3 translationOffset = GetTranslationOffset();

        m_translationManipulators = AZStd::make_shared<AzToolsFramework::TranslationManipulators>(
        AzToolsFramework::TranslationManipulators::Dimensions::Three, manipulatorSpace, nonUniformScale);

        m_translationManipulators->SetLocalTransform(AZ::Transform::CreateTranslation(translationOffset));
        m_translationManipulators->SetLineBoundWidth(AzToolsFramework::ManipulatorLineBoundWidth());
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(m_translationManipulators.get());

        auto mouseMoveHandlerFn = [this, transformScale{ m_translationManipulators->GetSpace().GetUniformScale() }](const auto& action)
        {
            const AZ::Vector3 translationOffset = GetTranslationOffset();
            const AZ::Transform manipulatorLocalTransform = AZ::Transform::CreateTranslation(translationOffset);
            const AZ::Vector3 manipulatorPosition = GetPositionInManipulatorFrame(transformScale, manipulatorLocalTransform, action);
            SetTranslationOffset(translationOffset + manipulatorPosition);
            UpdateManipulators();
        };

        m_translationManipulators->InstallLinearManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        m_translationManipulators->InstallPlanarManipulatorMouseMoveCallback(mouseMoveHandlerFn);

        m_translationManipulators->Register(manipulatorManagerId);
    }

    void ShapeTranslationOffsetViewportEdit::Teardown()
    {
        m_translationManipulators->Unregister();
        m_translationManipulators.reset();
    }

    void ShapeTranslationOffsetViewportEdit::UpdateManipulators()
    {
        m_translationManipulators->SetSpace(GetManipulatorSpace());
        m_translationManipulators->SetLocalTransform(AZ::Transform::CreateTranslation(GetTranslationOffset()));
        m_translationManipulators->SetNonUniformScale(GetNonUniformScale());
        m_translationManipulators->SetBoundsDirty();
    }

    void ShapeTranslationOffsetViewportEdit::ResetValuesImpl()
    {
        SetTranslationOffset(AZ::Vector3::CreateZero());
    }
} // namespace AzToolsFramework

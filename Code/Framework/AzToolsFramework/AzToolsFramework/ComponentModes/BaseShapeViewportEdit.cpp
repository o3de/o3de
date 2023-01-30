/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzToolsFramework/ComponentModes/BaseShapeViewportEdit.h>

namespace AzToolsFramework
{
    void BaseShapeViewportEdit::InstallGetManipulatorSpace(AZStd::function<AZ::Transform()> getManipulatorSpace)
    {
        m_getManipulatorSpace = AZStd::move(getManipulatorSpace);
    }

    void BaseShapeViewportEdit::InstallGetNonUniformScale(AZStd::function<AZ::Vector3()> getNonUniformScale)
    {
        m_getNonUniformScale = AZStd::move(getNonUniformScale);
    }

    void BaseShapeViewportEdit::InstallGetTranslationOffset(AZStd::function<AZ::Vector3()> getTranslationOffset)
    {
        m_getTranslationOffset = AZStd::move(getTranslationOffset);
    }

    void BaseShapeViewportEdit::InstallSetTranslationOffset(AZStd::function<void(const AZ::Vector3&)> setTranslationOffset)
    {
        m_setTranslationOffset = AZStd::move(setTranslationOffset);
    }

    AZ::Transform BaseShapeViewportEdit::GetManipulatorSpace() const
    {
        if (m_getManipulatorSpace)
        {
            return m_getManipulatorSpace();
        }
        AZ_ErrorOnce("BaseShapeViewportEdit", false, "No implementation provided for GetManipulatorSpace");
        return AZ::Transform::CreateIdentity();
    }

    AZ::Vector3 BaseShapeViewportEdit::GetNonUniformScale() const
    {
        if (m_getNonUniformScale)
        {
            return m_getNonUniformScale();
        }
        AZ_ErrorOnce("BaseShapeViewportEdit", false, "No implementation provided for GetNonUniformScale");
        return AZ::Vector3::CreateOne();
    }

    AZ::Vector3 BaseShapeViewportEdit::GetTranslationOffset() const
    {
        if (m_getTranslationOffset)
        {
            return m_getTranslationOffset();
        }
        AZ_ErrorOnce("BaseShapeViewportEdit", false, "No implementation provided for GetTranslationOffset");
        return AZ::Vector3::CreateZero();
    }

    void BaseShapeViewportEdit::SetTranslationOffset(const AZ::Vector3& translationOffset)
    {
        if (m_setTranslationOffset)
        {
            m_setTranslationOffset(translationOffset);
        }
        else
        {
            AZ_ErrorOnce("BaseShapeViewportEdit", false, "No implementation provided for SetTranslationOffset");
        }
    }
} // namespace AzToolsFramework

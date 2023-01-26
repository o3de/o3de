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
    void BaseShapeViewportEdit::InstallGetManipulatorSpaceFunction(
        const AZStd::function<AZ::Transform()>& getManipulatorSpaceFunction)
    {
        m_getManipulatorSpaceFunction = getManipulatorSpaceFunction;
    }

    void BaseShapeViewportEdit::InstallGetNonUniformScaleFunction(
        const AZStd::function<AZ::Vector3()>& getNonUniformScaleFunction)
    {
        m_getNonUniformScaleFunction = getNonUniformScaleFunction;
    }

    void BaseShapeViewportEdit::InstallGetTranslationOffsetFunction(
        const AZStd::function<AZ::Vector3()>& getTranslationOffsetFunction)
    {
        m_getTranslationOffsetFunction = getTranslationOffsetFunction;
    }

    void BaseShapeViewportEdit::InstallSetTranslationOffsetFunction(
        const AZStd::function<void(const AZ::Vector3)>& setTranslationOffsetFunction)
    {
        m_setTranslationOffsetFunction = setTranslationOffsetFunction;
    }

    AZ::Transform BaseShapeViewportEdit::GetManipulatorSpace() const
    {
        if (m_getManipulatorSpaceFunction)
        {
            return m_getManipulatorSpaceFunction();
        }
        AZ_WarningOnce("BaseShapeViewportEdit", false, "No implementation provided for GetManipulatorSpace");
        return AZ::Transform::CreateIdentity();
    }

    AZ::Vector3 BaseShapeViewportEdit::GetNonUniformScale() const
    {
        if (m_getNonUniformScaleFunction)
        {
            return m_getNonUniformScaleFunction();
        }
        AZ_WarningOnce("BaseShapeViewportEdit", false, "No implementation provided for GetNonUniformScale");
        return AZ::Vector3::CreateOne();
    }

    AZ::Vector3 BaseShapeViewportEdit::GetTranslationOffset() const
    {
        if (m_getTranslationOffsetFunction)
        {
            return m_getTranslationOffsetFunction();
        }
        AZ_WarningOnce("BaseShapeViewportEdit", false, "No implementation provided for GetTranslationOffset");
        return AZ::Vector3::CreateZero();
    }

    void BaseShapeViewportEdit::SetTranslationOffset(const AZ::Vector3& translationOffset)
    {
        if (m_setTranslationOffsetFunction)
        {
            m_setTranslationOffsetFunction(translationOffset);
        }
        else
        {
            AZ_WarningOnce("BaseShapeViewportEdit", false, "No implementation provided for SetTranslationOffset");
        }
    }
} // namespace AzToolsFramework

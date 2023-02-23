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

    void BaseShapeViewportEdit::InstallGetRotationOffset(AZStd::function<AZ::Quaternion()> getRotationOffset)
    {
        m_getRotationOffset = AZStd::move(getRotationOffset);
    }

    void BaseShapeViewportEdit::InstallSetTranslationOffset(AZStd::function<void(const AZ::Vector3&)> setTranslationOffset)
    {
        m_setTranslationOffset = AZStd::move(setTranslationOffset);
    }

    void BaseShapeViewportEdit::InstallBeginEditing(AZStd::function<void()> beginEditing)
    {
        m_beginEditing = AZStd::move(beginEditing);
    }

    void BaseShapeViewportEdit::InstallEndEditing(AZStd::function<void()> endEditing)
    {
        m_endEditing = AZStd::move(endEditing);
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
            return m_getNonUniformScale().GetMax(AZ::Vector3(AZ::MinTransformScale));
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

    AZ::Quaternion BaseShapeViewportEdit::GetRotationOffset() const
    {
        if (m_getRotationOffset)
        {
            return m_getRotationOffset();
        }
        AZ_ErrorOnce("BaseShapeViewportEdit", false, "No implementation provided for GetRotationOffset");
        return AZ::Quaternion::CreateIdentity();
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

    AZ::Transform BaseShapeViewportEdit::GetLocalTransform() const
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(GetRotationOffset(), GetTranslationOffset());
    }

    void BaseShapeViewportEdit::BeginEditing()
    {
        if (m_beginEditing)
        {
            m_beginEditing();
        }
    }

    void BaseShapeViewportEdit::EndEditing()
    {
        if (m_endEditing)
        {
            m_endEditing();
        }
    }

    void BaseShapeViewportEdit::BeginUndoBatch(const char* label)
    {
        if (!m_entityIds.empty())
        {
            ToolsApplicationRequests::Bus::BroadcastResult(
                m_undoBatch, &ToolsApplicationRequests::Bus::Events::BeginUndoBatch, label);

            for (const AZ::EntityId& entityId : m_entityIds)
            {
                ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
            }
        }
    }

    void BaseShapeViewportEdit::EndUndoBatch()
    {
        if (m_undoBatch)
        {
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::EndUndoBatch);
            m_undoBatch = nullptr;
        }
    }

    void BaseShapeViewportEdit::OnCameraStateChanged([[maybe_unused]] const AzFramework::CameraState& cameraState)
    {
    }

    void BaseShapeViewportEdit::ResetValues()
    {
        // manipulators handle undo batches for the main viewport themselves, but this function does not work via manipulators
        // so needs its own undo batch
        BeginUndoBatch("ViewportEdit Reset");
        // also provide a hook for other viewports to handle undo/redo, UI refresh, etc
        BeginEditing();
        ResetValuesImpl();
        ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
        EndEditing();
        EndUndoBatch();
    }

    void BaseShapeViewportEdit::AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityIds.insert(entityComponentIdPair.GetEntityId());
        AddEntityComponentIdPairImpl(entityComponentIdPair);
    }
} // namespace AzToolsFramework

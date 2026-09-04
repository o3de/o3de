/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "KeyUIControls.h"

#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"
#include "TrackViewKeyPropertiesDlg.h"

///////////////////////////////////////////////////////
template<class V>
IAnimTrack* CVectorKeyUIControlsBase<V>::GetCompoundTrackFromKeys(const CTrackViewKeyBundle& selectedKeys)
{
    if ((selectedKeys.GetKeyCount() < 1) || !selectedKeys.AreAllKeysOfSameType())
    {
        return nullptr;
    }

    const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);
    auto pTrack = keyHandle.GetTrack();
    if (!pTrack)
    {
        return nullptr;
    }

    if (!pTrack->IsCompoundTrack())
    {
        if (!pTrack->IsSubTrack())
        {
            return nullptr; // Simple track
        }
        if (const auto pParentNode = pTrack->GetParentNode())
        {
            if (pParentNode->GetNodeType() == ETrackViewNodeType::eTVNT_Track)
            {
                pTrack = static_cast<CTrackViewTrack*>(pParentNode);
            }
        }
    }

    if ((pTrack->GetValueType() != m_valueType) || !pTrack->GetAnimTrack())
    {
        return nullptr;
    }

    return pTrack->GetAnimTrack();
}

////////////////////////////////////////////////////////////////
bool CVectorKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    auto pAnimTrack = GetCompoundTrackFromKeys(selectedKeys);
    if (!pAnimTrack)
    {
        return false;
    }

    const auto keyTime = selectedKeys.GetKey(0).GetTime();
    m_vector = AZ::Vector3::CreateZero();
    pAnimTrack->GetValue(keyTime, m_vector, false);

    float fMin = -1.0f;
    float fMax = 1.0f;

    // Don't trigger an OnUIChange event, since this code is the one
    // updating the start/end ui elements, not the user setting new values.
    m_skipOnUIChange = true;

    mv_x = m_vector.GetX();
    pAnimTrack->GetSubTrack(0)->GetKeyValueRange(fMin, fMax);
    float step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_x->SetLimits(fMin, fMax, step, false, false);

    mv_y = m_vector.GetY();
    pAnimTrack->GetSubTrack(1)->GetKeyValueRange(fMin, fMax);
    step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_y->SetLimits(fMin, fMax, step, false, false);

    mv_z = m_vector.GetZ();
    pAnimTrack->GetSubTrack(2)->GetKeyValueRange(fMin, fMax);
    step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_z->SetLimits(fMin, fMax, step, false, false);

    m_skipOnUIChange = false;
    return true;
}

void CVectorKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    auto pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (m_skipOnUIChange || !pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    auto pAnimTrack = GetCompoundTrackFromKeys(selectedKeys);
    if (!pAnimTrack)
    {
        return;
    }

    const auto keyTime = selectedKeys.GetKey(0).GetTime();
    m_vector = AZ::Vector3::CreateZero();
    pAnimTrack->GetValue(keyTime, m_vector, false);

    AZ::Vector3 newVector(m_vector);
    if (pVar == mv_x.GetVar())
    {
        newVector.SetX(mv_x);
    }
    else if (pVar == mv_y.GetVar())
    {
        newVector.SetY(mv_y);
    }
    else if (pVar == mv_z.GetVar())
    {
        newVector.SetZ(mv_z);
    }

    if (newVector.IsClose(m_vector, AZ::Constants::Tolerance))
    {
        return;
    }

    bool isDuringUndo = false;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

    if (isDuringUndo)
    {
        pAnimTrack->SetValue(keyTime, newVector, false);
    }
    else
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
        pAnimTrack->SetValue(keyTime, newVector, false);
        undoBatch.MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }
}

///////////////////////////////////////////////////
bool CRgbKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    auto pAnimTrack = GetCompoundTrackFromKeys(selectedKeys);
    if (!pAnimTrack)
    {
        return false;
    }

    const auto keyTime = selectedKeys.GetKey(0).GetTime();
    m_vector = AZ::Vector3::CreateZero();
    pAnimTrack->GetValue(keyTime, m_vector, false);

    float fMin = -1.0f;
    float fMax = 1.0f;
    pAnimTrack->GetKeyValueRange(fMin, fMax);
    float step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);

    // Don't trigger an OnUIChange event, since this code is the one
    // updating the start/end ui elements, not the user setting new values.
    m_skipOnUIChange = true;

    mv_x = m_vector.GetX();
    mv_x.GetVar()->SetLimits(fMin, fMax, step, true, true);

    mv_y = m_vector.GetY();
    mv_y.GetVar()->SetLimits(fMin, fMax, step, true, true);

    mv_z = m_vector.GetZ();
    mv_z.GetVar()->SetLimits(fMin, fMax, step, true, true);

    m_skipOnUIChange = false;

    return true;
}

///////////////////////////////////////////////////
bool CQuatKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    auto pAnimTrack = GetCompoundTrackFromKeys(selectedKeys);
    if (!pAnimTrack)
    {
        return false;
    }

    const auto keyTime = selectedKeys.GetKey(0).GetTime();
    m_vector = AZ::Vector3::CreateZero();
    pAnimTrack->GetValue(keyTime, m_vector, false);

    float fMin = -1.0f;
    float fMax = 1.0f;

    // Don't trigger an OnUIChange event, since this code is the one
    // updating the start/end ui elements, not the user setting new values.
    m_skipOnUIChange = true;

    mv_x = m_vector.GetX();
    pAnimTrack->GetSubTrack(0)->GetKeyValueRange(fMin, fMax);
    float step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_x->SetLimits(fMin, fMax, step, true, true);

    mv_y = m_vector.GetY();
    pAnimTrack->GetSubTrack(1)->GetKeyValueRange(fMin, fMax);
    step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_y->SetLimits(fMin, fMax, step, true, true);

    mv_z = m_vector.GetZ();
    pAnimTrack->GetSubTrack(2)->GetKeyValueRange(fMin, fMax);
    step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_y->SetLimits(fMin, fMax, step, true, true);

    m_skipOnUIChange = false;

    return true;
}


///////////////////////////////////////////////////
bool CVector4KeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    auto pAnimTrack = GetCompoundTrackFromKeys(selectedKeys);
    if (!pAnimTrack)
    {
        return false;
    }

    const auto keyTime = selectedKeys.GetKey(0).GetTime();
    m_vector = AZ::Vector4::CreateZero();
    pAnimTrack->GetValue(keyTime, m_vector, false);

    float fMin = -1.0f;
    float fMax = 1.0f;

    // Don't trigger an OnUIChange event, since this code is the one
    // updating the start/end ui elements, not the user setting new values.
    m_skipOnUIChange = true;

    mv_x = m_vector.GetX();
    pAnimTrack->GetSubTrack(0)->GetKeyValueRange(fMin, fMax);
    float step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_x->SetLimits(fMin, fMax, step, false, false);

    mv_y = m_vector.GetY();
    pAnimTrack->GetSubTrack(1)->GetKeyValueRange(fMin, fMax);
    step = AZStd::max((fMax - fMin) / 100.f, 0.01f);
    mv_y->SetLimits(fMin, fMax, step, false, false);

    mv_z = m_vector.GetZ();
    pAnimTrack->GetSubTrack(2)->GetKeyValueRange(fMin, fMax);
    step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_z->SetLimits(fMin, fMax, step, false, false);

    mv_w = m_vector.GetW();
    pAnimTrack->GetSubTrack(3)->GetKeyValueRange(fMin, fMax);
    step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);
    mv_w->SetLimits(fMin, fMax, step, false, false);

    m_skipOnUIChange = false;

    return true;
}

void CVector4KeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    auto pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (m_skipOnUIChange || !pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    auto pAnimTrack = GetCompoundTrackFromKeys(selectedKeys);
    if (!pAnimTrack)
    {
        return;
    }

    const auto keyTime = selectedKeys.GetKey(0).GetTime();
    m_vector = AZ::Vector4::CreateZero();
    pAnimTrack->GetValue(keyTime, m_vector, false);

    AZ::Vector4 newVector(m_vector);
    if (pVar == mv_x.GetVar())
    {
        newVector.SetX(mv_x);
    }
    else if (pVar == mv_y.GetVar())
    {
        newVector.SetY(mv_y);
    }
    else if (pVar == mv_z.GetVar())
    {
        newVector.SetZ(mv_z);
    }
    else if (pVar == mv_w.GetVar())
    {
        newVector.SetW(mv_w);
    }
    if (newVector.IsClose(m_vector, AZ::Constants::Tolerance))
    {
        return;
    }

    bool isDuringUndo = false;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

    if (isDuringUndo)
    {
        pAnimTrack->SetValue(keyTime, newVector, false);
    }
    else
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
        pAnimTrack->SetValue(keyTime, newVector, false);
        undoBatch.MarkEntityDirty(pSequence->GetSequenceComponentEntityId());
    }
}

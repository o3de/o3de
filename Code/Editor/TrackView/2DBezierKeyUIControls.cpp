/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// Editor
#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"

bool C2DBezierKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        float fMin = 0.0f;
        float fMax = 0.0f;

        const CTrackViewTrack* pTrack = keyHandle.GetTrack();
        pTrack->GetKeyValueRange(fMin, fMax);

        if (fMin != fMax)
        {
            float curMin, curMax, step;
            bool  curMinHardLimit, curMaxHardLimit;

            // need to call GetLimits to retrieve/maintain *HardLimit boolean values
            mv_value.GetVar()->GetLimits(curMin, curMax, step, curMinHardLimit, curMaxHardLimit);

            step = ReflectedPropertyItem::ComputeSliderStep(fMin, fMax);

            mv_value.GetVar()->SetLimits(fMin, fMax, step, curMinHardLimit, curMaxHardLimit);
        }
        else
        {
            mv_value.GetVar()->ClearLimits();
        }

        EAnimCurveType trType = keyHandle.GetTrack()->GetCurveType();
        if (trType == eAnimCurveType_BezierFloat)
        {
            I2DBezierKey bezierKey;
            keyHandle.GetKey(&bezierKey);

            m_skipOnUIChange = true;
            SyncValue(mv_value, bezierKey.value.y, true);
            m_skipOnUIChange = false;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void C2DBezierKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType() || m_skipOnUIChange)
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);
        EAnimCurveType trType = keyHandle.GetTrack()->GetCurveType();

        if (trType == eAnimCurveType_BezierFloat)
        {
            I2DBezierKey bezierKey;
            keyHandle.GetKey(&bezierKey);

            SyncValue(mv_value, bezierKey.value.y, false, pVar);

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&bezierKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&bezierKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}

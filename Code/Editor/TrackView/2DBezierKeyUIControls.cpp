/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// Editor
#include "TrackViewKeyPropertiesDlg.h"
#include "Controls/ReflectedPropertyControl/ReflectedPropertyItem.h"


//////////////////////////////////////////////////////////////////////////
class C2DBezierKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    C2DBezierKeyUIControls()
    : m_skipOnUIChange(false)
    {}

    CSmartVariableArray mv_table;
    CSmartVariable<float> mv_value;

    void OnCreateVars() override
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_value, "Value");
    }
    bool SupportTrackType([[maybe_unused]] const CAnimParamType& paramType, EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const override
    {
        return trackType == eAnimCurveType_BezierFloat;
    }
    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    unsigned int GetPriority() const override { return 0; }

    static const GUID& GetClassID()
    {
        // {DBD76F4B-8EFC-45b6-AFB8-56F171FA150A}
        static const GUID guid =
        {
            0xdbd76f4b, 0x8efc, 0x45b6, { 0xaf, 0xb8, 0x56, 0xf1, 0x71, 0xfa, 0x15, 0xa }
        };
        return guid;
    }

    bool m_skipOnUIChange;
};

//////////////////////////////////////////////////////////////////////////
bool C2DBezierKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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

REGISTER_QT_CLASS_DESC(C2DBezierKeyUIControls, "TrackView.KeyUI.2DBezier", "TrackViewKeyUI");

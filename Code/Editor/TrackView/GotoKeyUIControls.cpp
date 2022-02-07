/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "TrackViewKeyPropertiesDlg.h"


//////////////////////////////////////////////////////////////////////////
class CGotoKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariable<float> mv_command;

    void OnCreateVars() override
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_command, "Goto Time");
    }
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const override
    {
        if (paramType == AnimParamType::Goto)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {3E9D2C57-BFB1-42f9-82AC-A393C1062634}
        static const GUID guid =
        {
            0x9b79c8b6, 0xe332, 0x4b9b, { 0xb2, 0x63, 0xef, 0x7e, 0x82, 0x7, 0xa4, 0x47 }
        };
        return guid;
    }
};

//////////////////////////////////////////////////////////////////////////
bool CGotoKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Goto)
        {
            IDiscreteFloatKey discreteFloatKey;
            keyHandle.GetKey(&discreteFloatKey);

            mv_command = discreteFloatKey.m_fValue;

            bAssigned = true;
        }
    }
    return bAssigned;
}
//////////////////////////////////////////////////////////////////////////
// Called when UI variable changes.
void CGotoKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Goto)
        {
            IDiscreteFloatKey discreteFloatKey;

            keyHandle.GetKey(&discreteFloatKey);
            SyncValue(mv_command, discreteFloatKey.m_fValue, false, pVar);

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&discreteFloatKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&discreteFloatKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}

REGISTER_QT_CLASS_DESC(CGotoKeyUIControls, "TrackView.KeyUI.Goto", "TrackViewKeyUI");

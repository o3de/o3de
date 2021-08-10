/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>  // for AnimParamType

// Editor
#include "TrackViewKeyPropertiesDlg.h"  // for CTrackViewKeyUIControls


//////////////////////////////////////////////////////////////////////////
class CConsoleKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariable<QString> mv_command;

    void OnCreateVars() override
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_command, "Command");
    }
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const override
    {
        return paramType == AnimParamType::Console;
    }
    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {3E9D2C57-BFB1-42f9-82AC-A393C1062634}
        static const GUID guid =
        {
            0x3e9d2c57, 0xbfb1, 0x42f9, { 0x82, 0xac, 0xa3, 0x93, 0xc1, 0x6, 0x26, 0x34 }
        };
        return guid;
    }
};

//////////////////////////////////////////////////////////////////////////
bool CConsoleKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Console)
        {
            IConsoleKey consoleKey;
            keyHandle.GetKey(&consoleKey);

            mv_command = ((QString)consoleKey.command.c_str()).toUtf8().data();

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CConsoleKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Console)
        {
            IConsoleKey consoleKey;
            keyHandle.GetKey(&consoleKey);

            if (pVar == mv_command.GetVar())
            {
                consoleKey.command = ((QString)mv_command).toUtf8().data();
            }

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&consoleKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&consoleKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }

        }
    }
}

REGISTER_QT_CLASS_DESC(CConsoleKeyUIControls, "TrackView.KeyUI.Console", "TrackViewKeyUI");

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewKeyPropertiesDlg.h"  // for CTrackViewKeyUIControls

#include <CryCommon/Maestro/Types/AnimParamType.h>  // AnimParamType

//////////////////////////////////////////////////////////////////////////
class CEventKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableArray mv_deprecated;

    CSmartVariableEnum<QString> mv_animation;
    CSmartVariableEnum<QString> mv_event;
    CSmartVariable<QString> mv_value;
    CSmartVariable<bool> mv_notrigger_in_scrubbing;

    void OnCreateVars() override
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_event, "Event");
        AddVariable(mv_table, mv_value, "Value");
        AddVariable(mv_table, mv_notrigger_in_scrubbing, "No trigger in scrubbing");
        AddVariable(mv_deprecated, "Deprecated");
        AddVariable(mv_deprecated, mv_animation, "Animation");
    }
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const override
    {
        return paramType == AnimParamType::Event;
    }
    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {ED5A2023-EDE1-4a47-BBE6-7D7BA0E4001D}
        static const GUID guid =
        {
            0xed5a2023, 0xede1, 0x4a47, { 0xbb, 0xe6, 0x7d, 0x7b, 0xa0, 0xe4, 0x0, 0x1d }
        };
        return guid;
    }

private:
};

//////////////////////////////////////////////////////////////////////////
bool CEventKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Event)
        {
            mv_event.SetEnumList(nullptr);
            mv_animation.SetEnumList(nullptr);

            // Add <None> for empty, unset event
            mv_event->AddEnumItem(QObject::tr("<None>"), "");
            mv_animation->AddEnumItem(QObject::tr("<None>"), "");

            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            mv_event = eventKey.event.c_str();
            mv_value = eventKey.eventValue.c_str();
            mv_animation = eventKey.animation.c_str();
            mv_notrigger_in_scrubbing = eventKey.bNoTriggerInScrubbing;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CEventKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Event)
        {
            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            QByteArray event, value, animation;
            event = static_cast<QString>(mv_event).toUtf8();
            value = static_cast<QString>(mv_value).toUtf8();
            animation = static_cast<QString>(mv_animation).toUtf8();

            if (pVar == mv_event.GetVar())
            {
                eventKey.event = event.data();
            }
            if (pVar == mv_value.GetVar())
            {
                eventKey.eventValue = value.data();
            }
            if (pVar == mv_animation.GetVar())
            {
                eventKey.animation = animation.data();
            }
            SyncValue(mv_notrigger_in_scrubbing, eventKey.bNoTriggerInScrubbing, false, pVar);

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&eventKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&eventKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}

REGISTER_QT_CLASS_DESC(CEventKeyUIControls, "TrackView.KeyUI.Event", "TrackViewKeyUI");

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
#include "TVEventsDialog.h"


//////////////////////////////////////////////////////////////////////////
class CTrackEventKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariableEnum<QString> mv_event;
    CSmartVariable<QString> mv_value;

    void OnCreateVars() override
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_event, "Track Event");
        mv_event->SetFlags(mv_event->GetFlags() | IVariable::UI_UNSORTED);
        AddVariable(mv_table, mv_value, "Value");
    }
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const override
    {
        return paramType == AnimParamType::TrackEvent;
    }
    bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys) override;
    void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys) override;

    unsigned int GetPriority() const override { return 1; }

    static const GUID& GetClassID()
    {
        // {F7D002EB-1FEA-46fa-B857-FC2B1B990B7F}
        static const GUID guid =
        {
            0xf7d002eb, 0x1fea, 0x46fa, { 0xb8, 0x57, 0xfc, 0x2b, 0x1b, 0x99, 0xb, 0x7f }
        };
        return guid;
    }

private:
    void OnEventEdit();
    void BuildEventDropDown(QString& curEvent, const QString& addedEvent = "");

    QString m_lastEvent;

    static const char* GetAddEventString()
    {
        static const char* addEventString = "Add a new event...";

        return addEventString;
    }
};

//////////////////////////////////////////////////////////////////////////
bool CTrackEventKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::TrackEvent)
        {
            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            // Provide builder with current event value to ensure
            // dropdown is displayed properly and value is updated if not found
            QString event = eventKey.event.c_str();
            BuildEventDropDown(event);

            mv_event = event;
            mv_value = eventKey.eventValue.c_str();

            bAssigned = true;
        }
    }

    m_lastEvent = mv_event;

    return bAssigned;
}

// Called when UI variable changes.
void CTrackEventKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    if (mv_event == GetAddEventString())
    {
        mv_event = m_lastEvent;
        OnEventEdit();
        return;
    }

    if (mv_event == "___spacer___")
    {
        mv_event = m_lastEvent;
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::TrackEvent)
        {
            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            QByteArray event, value;
            event = static_cast<QString>(mv_event).toUtf8();
            value = static_cast<QString>(mv_value).toUtf8();

            if (pVar == mv_event.GetVar())
            {
                eventKey.event = event.data();
            }
            if (pVar == mv_value.GetVar())
            {
                eventKey.eventValue = value.data();
            }
            eventKey.animation = "";
            eventKey.duration = 0;

            keyHandle.SetKey(&eventKey);
        }
    }

    m_lastEvent = mv_event;
}

void CTrackEventKeyUIControls::OnEventEdit()
{
    // Create dialog
    CTVEventsDialog dlg;
    dlg.exec();

    QString event = mv_event;
    BuildEventDropDown(event, dlg.GetLastAddedEvent());

    // The step below is necessary to make the event drop-down up-to-date.
    mv_event.GetVar()->EnableNotifyWithoutValueChange(true);
    mv_event = event;
    mv_event.GetVar()->EnableNotifyWithoutValueChange(false);
}

void CTrackEventKeyUIControls::BuildEventDropDown(QString& curEvent, const QString& addedEvent)
{
    if (CAnimationContext* context = GetIEditor()->GetAnimation())
    {
        CTrackViewSequence* sequence = context->GetSequence();

        if (sequence)
        {
            bool curEventExists = false;
            bool addedEventExists = false;
            mv_event.SetEnumList(nullptr);
            const int eventCount = sequence->GetTrackEventsCount();

            // Need to check if event exists before adding all events
            // This handles the case where the current event got deleted in the dialog but no new events were added
            for (int i = 0; i < eventCount; ++i)
            {
                const char* trackEvent = sequence->GetTrackEvent(i);

                if (curEvent == trackEvent)
                {
                    curEventExists = true;
                }
                if (addedEvent == trackEvent)
                {
                    addedEventExists = true;
                }
                if (curEventExists && addedEventExists)
                {
                    break;
                }
            }

            if (!curEventExists)
            {
                if (addedEventExists)
                {
                    // Set added event if key not set
                    curEvent = addedEvent;
                }
                else
                {
                    mv_event->AddEnumItem(QObject::tr("<None>"), "");
                    curEvent = "";
                }
            }

            // Add Events
            for (int i = 0; i < eventCount; ++i)
            {
                const char* trackEvent = sequence->GetTrackEvent(i);

                mv_event->AddEnumItem(trackEvent, trackEvent);
            }

            // Used as a spacer to make Add a new event... standout
            mv_event->AddEnumItem(QObject::tr(""), "___spacer___");

            // Add a new event... to open event editor when selected
            mv_event->AddEnumItem(QObject::tr(GetAddEventString()), GetAddEventString());
        }
    }
}

REGISTER_QT_CLASS_DESC(CTrackEventKeyUIControls, "TrackView.KeyUI.TrackEvent", "TrackViewKeyUI");

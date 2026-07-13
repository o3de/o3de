/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiAVTrackEventKeyUIControls.h"
#include "UiAVEventsDialog.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewTrack.h"

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewTrackEventKeyUIControls::OnCreateVars()
{
    AddVariable(mv_table, "Key Properties");
    AddVariable(mv_table, mv_event, "Track Event");
    mv_event->SetFlags(mv_event->GetFlags() | IVariable::UI_UNSORTED);
    AddVariable(mv_table, mv_value, "Value");
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrackEventKeyUIControls::SupportTrackType(const CUiAnimParamType& paramType, EUiAnimCurveType trackType, EUiAnimValue valueType) const
{
    AZ_UNUSED(trackType);
    AZ_UNUSED(valueType);

    return paramType == eUiAnimParamType_TrackEvent;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewTrackEventKeyUIControls::OnKeySelectionChange(CUiAnimViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CUiAnimViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        CUiAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == eUiAnimParamType_TrackEvent)
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
    return bAssigned;
}

// Called when UI variable changes.
void CUiAnimViewTrackEventKeyUIControls::OnUIChange(IVariable* pVar, CUiAnimViewKeyBundle& selectedKeys)
{
    CUiAnimViewSequence* pSequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();

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
        CUiAnimViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CUiAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == eUiAnimParamType_TrackEvent)
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

void CUiAnimViewTrackEventKeyUIControls::OnEventEdit()
{
    // Create dialog
    CUiAVEventsDialog dlg;
    dlg.exec();

    QString curEvent = mv_event;
    BuildEventDropDown(curEvent, dlg.GetLastAddedEvent());

    // The step below is necessary to make the event drop-down up-to-date.
    mv_event.GetVar()->EnableNotifyWithoutValueChange(true);
    mv_event = curEvent;
    mv_event.GetVar()->EnableNotifyWithoutValueChange(false);
}

void CUiAnimViewTrackEventKeyUIControls::BuildEventDropDown(QString& curEvent, const QString& addedEvent)
{
    CUiAnimViewSequence* sequence = CUiAnimViewSequenceManager::GetSequenceManager()->GetCurrentSequence();

    if (sequence)
    {
        bool curEventExists = false;
        bool addedEventExists = false;
        mv_event.SetEnumList(NULL);
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
                // Current event not found so set it to <None>
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

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
#include "KeyUIControls.h"
#include "TrackViewDialog.h"

//////////////////////////////////////////////////////////////////////////
bool CSequenceKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Sequence)
        {
            CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

            /////////////////////////////////////////////////////////////////////////////////
            // fill sequence comboBox with available sequences
            mv_sequence.SetEnumList(nullptr);

            // Insert '<None>' empty enum
            mv_sequence->AddEnumItem(QObject::tr("<None>"), CTrackViewDialog::GetEntityIdAsString(AZ::EntityId(AZ::EntityId::InvalidEntityId)));

            const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
            for (unsigned int i = 0; i < pSequenceManager->GetCount(); ++i)
            {
                CTrackViewSequence* pCurrentSequence = pSequenceManager->GetSequenceByIndex(i);
                bool bNotMe = pCurrentSequence != pSequence;
                bool bNotParent = !bNotMe || pCurrentSequence->IsAncestorOf(pSequence) == false;
                if (bNotMe && bNotParent)
                {
                    AZStd::string seqName = pCurrentSequence->GetName();

                    QString ownerIdString = CTrackViewDialog::GetEntityIdAsString(pCurrentSequence->GetSequenceComponentEntityId());
                    mv_sequence->AddEnumItem(seqName.c_str(), ownerIdString);
                }
            }

            /////////////////////////////////////////////////////////////////////////////////
            // fill Key Properties with selected key values
            ISequenceKey sequenceKey;
            keyHandle.GetKey(&sequenceKey);
            
            QString entityIdString = CTrackViewDialog::GetEntityIdAsString((sequenceKey.sequenceEntityId));
            mv_sequence = entityIdString;            
           
            mv_overrideTimes = sequenceKey.bOverrideTimes;
            if (!sequenceKey.bOverrideTimes)
            {
                IAnimSequence* pSequence2 = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(sequenceKey.sequenceEntityId);

                if (pSequence2)
                {
                    sequenceKey.fStartTime = pSequence2->GetTimeRange().start;
                    sequenceKey.fEndTime = pSequence2->GetTimeRange().end;
                }
                else
                {
                    sequenceKey.fStartTime = 0.0f;
                    sequenceKey.fEndTime = 0.0f;
                }
            }

            // Don't trigger an OnUIChange event, since this code is the one
            // updating the start/end ui elements, not the user setting new values.
            m_skipOnUIChange = true;
            mv_startTime = sequenceKey.fStartTime;
            mv_endTime = sequenceKey.fEndTime;
            m_skipOnUIChange = false;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CSequenceKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType() || m_skipOnUIChange)
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Sequence)
        {
            ISequenceKey sequenceKey;
            keyHandle.GetKey(&sequenceKey);

            AZ::EntityId seqOwnerId;
            if (pVar == mv_sequence.GetVar())
            {
                QString entityIdString = mv_sequence;
                seqOwnerId = AZ::EntityId(static_cast<AZ::u64>(entityIdString.toULongLong()));

                sequenceKey.szSelection.clear();            // clear deprecated legacy data
                sequenceKey.sequenceEntityId = seqOwnerId;
            }

            SyncValue(mv_overrideTimes, sequenceKey.bOverrideTimes, false, pVar);

            IAnimSequence* pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(seqOwnerId);

            if (!sequenceKey.bOverrideTimes)
            {
                if (pSequence)
                {
                    sequenceKey.fStartTime = pSequence->GetTimeRange().start;
                    sequenceKey.fEndTime = pSequence->GetTimeRange().end;
                }
                else
                {
                    sequenceKey.fStartTime = 0.0f;
                    sequenceKey.fEndTime = 0.0f;
                }
            }
            else
            {
                SyncValue(mv_startTime, sequenceKey.fStartTime, false, pVar);
                SyncValue(mv_endTime, sequenceKey.fEndTime, false, pVar);
            }

            sequenceKey.fDuration = sequenceKey.fEndTime - sequenceKey.fStartTime > 0 ? sequenceKey.fEndTime - sequenceKey.fStartTime : 0.0f;

            IMovieSystem* pMovieSystem = GetIEditor()->GetSystem()->GetIMovieSystem();

            if (pMovieSystem != nullptr)
            {
                pMovieSystem->SetStartEndTime(pSequence, sequenceKey.fStartTime, sequenceKey.fEndTime);
            }

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&sequenceKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&sequenceKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }

        }
    }
}

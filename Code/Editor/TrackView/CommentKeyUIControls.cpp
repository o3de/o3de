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
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"

bool CCommentKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::CommentText)
        {
            ICommentKey commentKey;
            keyHandle.GetKey(&commentKey);

            mv_comment = commentKey.m_strComment.c_str();
            mv_duration = commentKey.m_duration;
            mv_size = commentKey.m_size;
            mv_font = commentKey.m_strFont.c_str();
            mv_color = Vec3(commentKey.m_color.GetR(), commentKey.m_color.GetG(), commentKey.m_color.GetB());
            mv_align = commentKey.m_align;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CCommentKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (size_t keyIndex = 0, num = selectedKeys.GetKeyCount(); keyIndex < num; keyIndex++)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(static_cast<unsigned int>(keyIndex));

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::CommentText)
        {
            ICommentKey commentKey;
            keyHandle.GetKey(&commentKey);

            if (!pVar || pVar == mv_comment.GetVar())
            {
                commentKey.m_strComment = ((QString)mv_comment).toUtf8().data();
            }

            if (!pVar || pVar == mv_font.GetVar())
            {
                QString sFont = mv_font;
                commentKey.m_strFont = sFont.toUtf8().data();
            }

            if (!pVar || pVar == mv_align.GetVar())
            {
                commentKey.m_align = (ICommentKey::ETextAlign)((int)mv_align);
            }

            SyncValue(mv_duration, commentKey.m_duration, false, pVar);
            Vec3 color(commentKey.m_color.GetR(), commentKey.m_color.GetG(), commentKey.m_color.GetB());
            SyncValue(mv_color, color, false, pVar);
            commentKey.m_color.Set(color.x, color.y, color.z, commentKey.m_color.GetA());
            SyncValue(mv_size, commentKey.m_size, false, pVar);

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&commentKey);
            }
            else
            {
                // Let the AZ Undo system manage the nodes on the sequence entity
                AzToolsFramework::ScopedUndoBatch undoBatch("Change key");
                keyHandle.SetKey(&commentKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}

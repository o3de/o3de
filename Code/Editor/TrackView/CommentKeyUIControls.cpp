/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>  // for AnimParamType

// Editor
#include "TrackViewKeyPropertiesDlg.h"


//////////////////////////////////////////////////////////////////////////
class CCommentKeyUIControls
    : public CTrackViewKeyUIControls
{
public:
    CSmartVariableArray mv_table;
    CSmartVariable<QString> mv_comment;
    CSmartVariable<float> mv_duration;
    CSmartVariable<float> mv_size;
    CSmartVariable<Vec3> mv_color;
    CSmartVariableEnum<int> mv_align;
    CSmartVariableEnum<QString> mv_font;


    virtual void OnCreateVars()
    {
        AddVariable(mv_table, "Key Properties");
        AddVariable(mv_table, mv_comment, "Comment");
        AddVariable(mv_table, mv_duration, "Duration");

        mv_size->SetLimits(1.f, 10.f);
        AddVariable(mv_table, mv_size, "Size");

        AddVariable(mv_table, mv_color, "Color", IVariable::DT_COLOR);

        mv_align->SetEnumList(NULL);
        mv_align->AddEnumItem("Left", ICommentKey::eTA_Left);
        mv_align->AddEnumItem("Center", ICommentKey::eTA_Center);
        mv_align->AddEnumItem("Right", ICommentKey::eTA_Right);
        AddVariable(mv_table, mv_align, "Align");

        mv_font->SetEnumList(NULL);
        IFileUtil::FileArray fa;
        CFileUtil::ScanDirectory((Path::GetEditingGameDataFolder() + "/Fonts/").c_str(), "*.xml", fa, true);
        for (size_t i = 0; i < fa.size(); ++i)
        {
            string name = fa[i].filename.toUtf8().data();
            PathUtil::RemoveExtension(name);
            mv_font->AddEnumItem(name.c_str(), name.c_str());
        }
        AddVariable(mv_table, mv_font, "Font");
    }
    bool SupportTrackType(const CAnimParamType& paramType, [[maybe_unused]] EAnimCurveType trackType, [[maybe_unused]] AnimValueType valueType) const
    {
        return paramType == AnimParamType::CommentText;
    }
    virtual bool OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys);
    virtual void OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys);

    virtual unsigned int GetPriority() const { return 1; }

    static const GUID& GetClassID()
    {
        // {FA250B8B-FC2A-43b1-AF7A-8C3B6672B49D}
        static const GUID guid =
        {
            0xfa250b8b, 0xfc2a, 0x43b1, { 0xaf, 0x7a, 0x8c, 0x3b, 0x66, 0x72, 0xb4, 0x9d }
        };
        return guid;
    }
};

//////////////////////////////////////////////////////////////////////////
bool CCommentKeyUIControls::OnKeySelectionChange(CTrackViewKeyBundle& selectedKeys)
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
//////////////////////////////////////////////////////////////////////////
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
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

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

REGISTER_QT_CLASS_DESC(CCommentKeyUIControls, "TrackView.KeyUI.Comment", "TrackViewKeyUI");

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREUNDO_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREUNDO_H
#pragma once

#include "Undo/IUndoObject.h"

class CLensFlareItem;

class CUndoLensFlareItem
    : public IUndoObject
{
public:
    CUndoLensFlareItem(CLensFlareItem* pGroupItem, const QString& undoDescription = "Undo Lens Flare Tree");
    ~CUndoLensFlareItem();

protected:
    int GetSize()
    {
        return sizeof(*this);
    }
    QString GetDescription() { return m_undoDescription; };
    void Undo(bool bUndo);
    void Redo();

private:

    QString m_undoDescription;

    struct SData
    {
        SData()
        {
            m_pOptics = NULL;
        }

        QString m_selectedFlareItemName;
        bool m_bRestoreSelectInfo;
        IOpticsElementBasePtr m_pOptics;
    };

    QString m_flarePathName;
    SData m_Undo;
    SData m_Redo;

    void Restore(const SData& data);
};

class CUndoRenameLensFlareItem
    : public IUndoObject
{
public:

    CUndoRenameLensFlareItem(const QString& oldFullName, const QString& newFullName);

protected:

    int GetSize()
    {
        return sizeof(*this);
    }
    QString GetDescription() { return m_undoDescription; };
    void Undo(bool bUndo);
    void Redo();

private:

    QString m_undoDescription;

    struct SUndoDataStruct
    {
        QString m_oldFullItemName;
        QString m_newFullItemName;
    };

    void Rename(const SUndoDataStruct& data);

    SUndoDataStruct m_undo;
    SUndoDataStruct m_redo;
};

class CUndoLensFlareElementSelection
    : public IUndoObject
{
public:
    CUndoLensFlareElementSelection(CLensFlareItem* pLensFlareItem, const QString& flareTreeItemFullName, const QString& undoDescription = "Undo Lens Flare Element Tree");
    ~CUndoLensFlareElementSelection(){}

protected:
    int GetSize()
    {
        return sizeof(*this);
    }
    QString GetDescription() { return m_undoDescription; };
    void Undo(bool bUndo);
    void Redo();

private:

    QString m_undoDescription;

    QString m_flarePathNameForUndo;
    QString m_flareTreeItemFullNameForUndo;

    QString m_flarePathNameForRedo;
    QString m_flareTreeItemFullNameForRedo;
};

class CUndoLensFlareItemSelectionChange
    : public IUndoObject
{
public:
    CUndoLensFlareItemSelectionChange(const QString& fullLensFlareItemName, const QString& undoDescription = "Undo Lens Flare element selection");
    ~CUndoLensFlareItemSelectionChange(){}

protected:
    int GetSize()
    {
        return sizeof(*this);
    }
    QString GetDescription() { return m_undoDescription; };

    void Undo(bool bUndo);
    void Redo();

private:
    QString m_undoDescription;
    QString m_FullLensFlareItemNameForUndo;
    QString m_FullLensFlareItemNameForRedo;
};
#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREUNDO_H

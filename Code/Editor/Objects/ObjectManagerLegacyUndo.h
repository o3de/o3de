/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "Undo/IUndoObject.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include "Objects/BaseObject.h"

//////////////////////////////////////////////////////////////////////////
//! Undo New Object
class CUndoBaseObjectNew
    : public IUndoObject
{
public:
    CUndoBaseObjectNew(CBaseObject* object);

protected:
    int GetSize() override { return sizeof(*this); } // Return size of xml state.
    QString GetObjectName() override { return m_object->GetName(); }

    void Undo(bool bUndo) override;
    void Redo() override;

private:
    CBaseObjectPtr m_object;
    TGUIDRemap m_remapping;
    XmlNodeRef m_redo;
};

//////////////////////////////////////////////////////////////////////////
//! Undo Delete Object
class CUndoBaseObjectDelete
    : public IUndoObject
{
public:
    CUndoBaseObjectDelete(CBaseObject* object);

protected:
    int GetSize() override { return sizeof(*this); } // Return size of xml state.
    QString GetObjectName() override { return m_object->GetName(); }

    void Undo(bool bUndo) override;
    void Redo() override;

private:
    CBaseObjectPtr m_object;
    XmlNodeRef m_undo;
    TGUIDRemap m_remapping;
    bool m_bSelected;
};

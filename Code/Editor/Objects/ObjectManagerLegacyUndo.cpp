/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ObjectManagerLegacyUndo.h"

// AzToolsFramework
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

// Editor
#include "Include/IObjectManager.h"
#include "Objects/BaseObject.h"
#include "Objects/ObjectLoader.h"

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectNew implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectNew::CUndoBaseObjectNew(CBaseObject* object)
{
    m_object = object;
}

void CUndoBaseObjectNew::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo = XmlHelpers::CreateXmlNode("Redo");
        // Save current object state.
        CObjectArchive ar(GetIEditor()->GetObjectManager(), m_redo, false);
        ar.bUndo = true;
        m_object->Serialize(ar);
    }

    // Delete this object.
    GetIEditor()->DeleteObject(m_object);
}

void CUndoBaseObjectNew::Redo()
{
    if (!m_redo)
    {
        return;
    }

    IObjectManager* objectManager = GetIEditor()->GetObjectManager();
    {
        CObjectArchive ar(objectManager, m_redo, true);
        ar.bUndo = true;
        ar.MakeNewIds(false);
        ar.LoadObject(m_redo, m_object);
    }
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectDelete implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectDelete::CUndoBaseObjectDelete(CBaseObject* object)
{
    AZ_Assert(object, "Object does not exist");
    object->SetTransformDelegate(nullptr);
    m_object = object;

    // Save current object state.
    m_undo = XmlHelpers::CreateXmlNode("Undo");
    CObjectArchive ar(GetIEditor()->GetObjectManager(), m_undo, false);
    ar.bUndo = true;
    m_bSelected = m_object->IsSelected();
    m_object->Serialize(ar);
}

void CUndoBaseObjectDelete::Undo([[maybe_unused]] bool bUndo)
{
    IObjectManager* objectManager = GetIEditor()->GetObjectManager();
    {
        CObjectArchive ar(objectManager, m_undo, true);
        ar.bUndo = true;
        ar.MakeNewIds(false);
        ar.LoadObject(m_undo, m_object);
        m_object->ClearFlags(OBJFLAG_SELECTED);
    }
}

void CUndoBaseObjectDelete::Redo()
{
    // Delete this object.
    GetIEditor()->DeleteObject(m_object);
}

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

#include "EditorDefs.h"

#include "MaterialLibrary.h"

// Editor
#include "BaseLibraryItem.h"


//////////////////////////////////////////////////////////////////////////
// CMaterialLibrary implementation.
//////////////////////////////////////////////////////////////////////////
bool CMaterialLibrary::Save()
{
    return SaveLibrary("MaterialLibrary");
}

//////////////////////////////////////////////////////////////////////////
bool CMaterialLibrary::Load(const QString& filename)
{
    if (filename.isEmpty())
    {
        return false;
    }
    SetFilename(filename);
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
    if (!root)
    {
        return false;
    }

    Serialize(root, true);
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialLibrary::Serialize([[maybe_unused]] XmlNodeRef& root, [[maybe_unused]] bool bLoading)
{
    /*
    if (bLoading)
    {
        // Loading.
        CString name = GetName();
        root->getAttr( "Name",name );
        SetName( name );
        for (int i = 0; i < root->getChildCount(); i++)
        {
            XmlNodeRef itemNode = root->getChild(i);
            CMaterial *material = new CMaterial(itemNode->getAttr("Name"));
            AddItem( material );
            CBaseLibraryItem::SerializeContext ctx( itemNode,bLoading );
            material->Serialize( ctx );
        }
        SetModified(false);
    }
    else
    {
        // Saving.
        root->setAttr( "Name",GetName() );
        root->setAttr( "SandboxVersion",(const char*)GetIEditor()->GetFileVersion().ToFullString() );
        // Serialize prototypes.
        for (int i = 0; i < GetItemCount(); i++)
        {
            CMaterial *pMtl = (CMaterial*)GetItem(i);
            // Save materials with parents under thier parent xml node.
            if (pMtl->GetParent())
                continue;

            XmlNodeRef itemNode = root->newChild( "Material" );
            CBaseLibraryItem::SerializeContext ctx( itemNode,bLoading );
            GetItem(i)->Serialize( ctx );
        }
    }
    */
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CMaterialLibrary::AddItem(IDataBaseItem* item, bool bRegister)
{
    CBaseLibraryItem* pLibItem = (CBaseLibraryItem*)item;
    // Check if item is already assigned to this library.
    if (pLibItem->GetLibrary() != this)
    {
        pLibItem->SetLibrary(this);
        if (bRegister)
        {
            m_pManager->RegisterItem(pLibItem);
        }
        m_items.push_back(pLibItem);
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CMaterialLibrary::GetItem(int index)
{
    assert(index >= 0 && index < m_items.size());
    return m_items[index];
}

//////////////////////////////////////////////////////////////////////////
void CMaterialLibrary::RemoveItem(IDataBaseItem* item)
{
    for (int i = 0; i < m_items.size(); i++)
    {
        if (m_items[i] == item)
        {
            m_items.erase(m_items.begin() + i);
            SetModified();
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseItem* CMaterialLibrary::FindItem(const QString& name)
{
    for (int i = 0; i < m_items.size(); i++)
    {
        if (QString::compare(m_items[i]->GetName(), name, Qt::CaseInsensitive) == 0)
        {
            return m_items[i];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialLibrary::RemoveAllItems()
{
    AddRef();
    for (int i = 0; i < m_items.size(); i++)
    {
        // Clear library item.
        m_items[i]->SetLibrary(NULL);
    }
    m_items.clear();
    Release();
}

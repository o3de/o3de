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

#include "LensFlareLibrary.h"

// Editor
#include "LensFlareItem.h"


bool CLensFlareLibrary::Save()
{
    return SaveLibrary("LensFlareLibrary");
}

bool CLensFlareLibrary::Load(const QString& filename)
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

void CLensFlareLibrary::Serialize(XmlNodeRef& root, bool bLoading)
{
    if (bLoading)
    {
        RemoveAllItems();
        QString name = GetName();
        root->getAttr("Name", name);
        SetName(name);
        for (int i = 0; i < root->getChildCount(); i++)
        {
            XmlNodeRef itemNode = root->getChild(i);
            CLensFlareItem* pLensFlareItem = new CLensFlareItem;
            AddItem(pLensFlareItem);
            CBaseLibraryItem::SerializeContext ctx(itemNode, bLoading);
            pLensFlareItem->Serialize(ctx);
        }
        SetModified(false);
        m_bNewLibrary = false;
    }
    else
    {
        root->setAttr("Name", GetName().toUtf8().data());

        for (int i = 0; i < GetItemCount(); i++)
        {
            CLensFlareItem* pItem = (CLensFlareItem*)GetItem(i);
            if(pItem)
            {
                CBaseLibraryItem::SerializeContext ctx(pItem->CreateXmlData(), bLoading);
                root->addChild(ctx.node);
                pItem->Serialize(ctx);
            }
        }
    }
}

IOpticsElementBasePtr CLensFlareLibrary::GetOpticsOfItem(const char* szflareName)
{
    IOpticsElementBasePtr pOptics = NULL;
    for (int i = 0; i < GetItemCount(); i++)
    {
        CLensFlareItem* pItem = (CLensFlareItem*)GetItem(i);

        if (pItem->GetFullName() == szflareName)
        {
            pOptics = pItem->GetOptics();
            break;
        }
    }

    return pOptics;
}


void CLensFlareLibrary::OnInternalVariableChange([[maybe_unused]] IVariable* pVar)
{
    SetModified(true);
}

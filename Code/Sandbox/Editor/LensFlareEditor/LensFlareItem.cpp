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

#include "LensFlareItem.h"

// Editor
#include "LensFlareUtil.h"
#include "LensFlareEditor.h"
#include "LensFlareUndo.h"
#include "Objects/EntityObject.h"


CLensFlareItem::CLensFlareItem()
{
    m_pOptics = NULL;
    CreateOptics();
}

CLensFlareItem::~CLensFlareItem()
{
}

void CLensFlareItem::Serialize(SerializeContext& ctx)
{
    if (ctx.bLoading)
    {
        CreateOptics();
        LensFlareUtil::FillOpticsFromXML(m_pOptics, ctx.node);

        for (int i = 0, iChildCount(ctx.node->getChildCount()); i < iChildCount; ++i)
        {
            AddChildOptics(m_pOptics, ctx.node->getChild(i));
        }

        CBaseLibraryItem::Serialize(ctx);
    }
    else
    {
        CBaseLibraryItem::Serialize(ctx);
    }
}

void CLensFlareItem::AddChildOptics(IOpticsElementBasePtr pParentOptics, const XmlNodeRef& pNode)
{
    if (pNode == NULL)
    {
        return;
    }

    if (pNode->getTag() && strcmp(pNode->getTag(), "FlareItem"))
    {
        return;
    }

    IOpticsElementBasePtr pOptics = LensFlareUtil::CreateOptics(pNode);
    if (pOptics == NULL)
    {
        return;
    }

    pParentOptics->AddElement(pOptics);

    for (int i = 0, iChildCount(pNode->getChildCount()); i < iChildCount; ++i)
    {
        AddChildOptics(pOptics, pNode->getChild(i));
    }
}

void CLensFlareItem::CreateOptics()
{
    m_pOptics = gEnv->pOpticsManager->Create(eFT_Root);
}

XmlNodeRef CLensFlareItem::CreateXmlData() const
{
    XmlNodeRef pRootNode = NULL;
    if (LensFlareUtil::CreateXmlData(m_pOptics, pRootNode))
    {
        pRootNode->setAttr("Name", m_pOptics->GetName().c_str());
        return pRootNode;
    }
    return NULL;
}

void CLensFlareItem::SetName(const QString& name)
{
    QString newNameWithGroup;
    QString newFullName;
    LensFlareUtil::GetExpandedItemNames(this, LensFlareUtil::GetGroupNameFromName(name), LensFlareUtil::GetShortName(name), newNameWithGroup, newFullName);

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoRenameLensFlareItem(GetFullName(), newFullName));
    }

    CBaseLibraryItem::SetName(name);

    if (m_pOptics)
    {
        m_pOptics->SetName(GetShortName().toUtf8().data());
    }
}

void CLensFlareItem::UpdateLights(IOpticsElementBasePtr pSrcOptics)
{
    QString srcFullOpticsName = GetFullName();
    bool bUpdateChildren = false;
    if (pSrcOptics == NULL)
    {
        if (m_pOptics == NULL)
        {
            return;
        }
        pSrcOptics = m_pOptics;
        bUpdateChildren = true;
    }

    std::vector<CEntityObject*> lightEntities;
    LensFlareUtil::GetLightEntityObjects(lightEntities);

    for (int i = 0, iLightSize(lightEntities.size()); i < iLightSize; ++i)
    {
        CEntityObject* pLightEntity = lightEntities[i];
        if (pLightEntity == NULL)
        {
            continue;
        }

        IOpticsElementBasePtr pTargetOptics = pLightEntity->GetOpticsElement();
        if (pTargetOptics == NULL)
        {
            continue;
        }

        QString targetFullOpticsName(pTargetOptics->GetName().c_str());
        if (srcFullOpticsName != targetFullOpticsName)
        {
            continue;
        }

        QString srcOpticsName(pSrcOptics->GetName().c_str());
        IOpticsElementBasePtr pFoundOptics = NULL;
        if (LensFlareUtil::GetShortName(targetFullOpticsName) == srcOpticsName)
        {
            pFoundOptics = pTargetOptics;
        }
        else
        {
            pFoundOptics = LensFlareUtil::FindOptics(pTargetOptics, srcOpticsName);
        }

        if (pFoundOptics == NULL)
        {
            continue;
        }
        if (pFoundOptics->GetType() != pSrcOptics->GetType())
        {
            continue;
        }

        LensFlareUtil::CopyOptics(pSrcOptics, pFoundOptics, bUpdateChildren);
    }
}

void CLensFlareItem::ReplaceOptics(IOpticsElementBasePtr pNewData)
{
    if (pNewData == NULL)
    {
        return;
    }
    if (pNewData->GetType() != eFT_Root)
    {
        return;
    }
    CreateOptics();
    LensFlareUtil::CopyOptics(pNewData, m_pOptics);
    m_pOptics->SetName(GetFullName().toUtf8().data());
    UpdateLights();

    CLensFlareEditor* pLensFlareEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pLensFlareEditor == NULL)
    {
        return;
    }
    if (this != pLensFlareEditor->GetSelectedLensFlareItem())
    {
        return;
    }
    pLensFlareEditor->UpdateLensFlareItem(this);
    pLensFlareEditor->RemovePropertyItems();
}

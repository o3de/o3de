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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREITEM_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREITEM_H
#pragma once

class IOpticsElementBase;
class CEntityObject;

#include "BaseLibraryItem.h"
#include "Include/IDataBaseItem.h"

class CLensFlareItem
    : public CBaseLibraryItem
{
public:

    CLensFlareItem();
    ~CLensFlareItem();

    EDataBaseItemType GetType() const
    {
        return EDB_TYPE_FLARE;
    }

    void SetName(const QString& name);
    void Serialize(SerializeContext& ctx);

    void CreateOptics();

    IOpticsElementBasePtr GetOptics() const
    {
        return m_pOptics;
    }

    void ReplaceOptics(IOpticsElementBasePtr pNewData);

    XmlNodeRef CreateXmlData() const;
    void UpdateLights(IOpticsElementBasePtr pSrcOptics = NULL);

private:

    void GetLightEntityObjects(std::vector<CEntityObject*>& outEntityLights) const;
    static void AddChildOptics(IOpticsElementBasePtr pParentOptics, const XmlNodeRef& pNode);

    IOpticsElementBasePtr m_pOptics;
};
#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREITEM_H

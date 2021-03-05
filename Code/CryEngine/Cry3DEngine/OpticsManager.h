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

#ifndef CRYINCLUDE_CRY3DENGINE_OPTICSMANAGER_H
#define CRYINCLUDE_CRY3DENGINE_OPTICSMANAGER_H
#pragma once

#include "IFlares.h"

class COpticsManager
    : public Cry3DEngineBase
    , public IOpticsManager
{
public:

    ~COpticsManager(){}

    void Reset();

    IOpticsElementBase* Create(EFlareType type) const;
    IOpticsElementBase* GetOptics(int nIndex);

    bool Load(const char* fullFlareName, int& nOutIndex, bool forceReload = false);
    bool Load(XmlNodeRef& rootNode, int& nOutIndex);
    bool AddOptics(IOpticsElementBase* pOptics, const char* name, int& nOutNewIndex, bool allowReplace = false);
    bool Rename(const char* fullFlareName, const char* newFullFlareName);

    void GetMemoryUsage(ICrySizer* pSizer) const;
    void Invalidate();

private:
    IOpticsElementBase* ParseOpticsRecursively(IOpticsElementBase* pParentOptics, XmlNodeRef& node) const;
    EFlareType GetFlareType(const char* typeStr) const;
    int FindOpticsIndex(const char* fullFlareName) const;

private:
    std::vector<IOpticsElementBasePtr> m_OpticsList;
    std::map<string, int> m_OpticsMap;
    // All flare list which has already been searched for.
    std::set<string> m_SearchedOpticsSet;
};

#endif // CRYINCLUDE_CRY3DENGINE_OPTICSMANAGER_H

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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSGROUP_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSGROUP_H
#pragma once

#include "OpticsElement.h"

class COpticsGroup
    : public COpticsElement
{
protected:
    std::vector<IOpticsElementBasePtr> children;
    void _init();

public:

    COpticsGroup(const char* name = "[Unnamed_Group]")
        : COpticsElement(name) { _init(); };
    COpticsGroup(const char* name, COpticsElement* elem, ...);
    virtual ~COpticsGroup(){}

    COpticsGroup& Add(IOpticsElementBase* pElement);
    void Remove(int i);
    void RemoveAll();
    virtual int GetElementCount() const;
    IOpticsElementBase* GetElementAt(int  i) const;

    void AddElement(IOpticsElementBase* pElement) {   Add(pElement);    }
    void InsertElement(int nPos, IOpticsElementBase* pElement);
    void SetElementAt(int i, IOpticsElementBase* elem);
    void Invalidate();

    bool IsGroup() const { return true; }
    void validateChildrenGlobalVars(SAuxParams& aux);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual EFlareType GetType() { return eFT_Group; }
    virtual void validateGlobalVars(SAuxParams& aux);

    void Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);

#if defined(FLARES_SUPPORT_EDITING)
    virtual void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
#endif

public:
    static COpticsGroup predef_simpleCamGhost;
    static COpticsGroup predef_cheapCamGhost;
    static COpticsGroup predef_multiGlassGhost;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSGROUP_H

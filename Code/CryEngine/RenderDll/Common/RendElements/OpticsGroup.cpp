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

#include "RenderDll_precompiled.h"
#include "../CryNameR.h"
#include "../../XRenderD3D9/DriverD3D.h"
#include "../Textures/Texture.h"
#include "OpticsGroup.h"

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&COpticsGroup::FUNC_NAME)
void COpticsGroup::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);
}
#undef MFPtr
#endif

void COpticsGroup::_init()
{
    SetSize(1.f);
    SetAutoRotation(true);
}

COpticsGroup::COpticsGroup(const char* name, COpticsElement* ghost, ...)
    : COpticsElement(name)
{
    _init();

    va_list arg;
    va_start(arg, ghost);

    COpticsElement* curArg;
    while ((curArg = va_arg(arg, COpticsElement*)) != NULL)
    {
        Add(curArg);
    }

    va_end(arg);
}

COpticsGroup& COpticsGroup::Add(IOpticsElementBase* pElement)
{
    children.push_back(pElement);
    ((COpticsElement*)&*pElement)->SetParent(this);
    return *this;
}

void COpticsGroup::InsertElement(int nPos, IOpticsElementBase* pElement)
{
    children.insert(children.begin() + nPos, pElement);
    ((COpticsElement*)&*pElement)->SetParent(this);
}

void COpticsGroup::Remove(int i)
{
    children.erase(children.begin() + i);
}

void COpticsGroup::RemoveAll()
{
    children.clear();
}

int COpticsGroup::GetElementCount() const { return children.size(); }

IOpticsElementBase* COpticsGroup::GetElementAt(int i) const { return children.at(i);  }

void COpticsGroup::SetElementAt(int i, IOpticsElementBase* elem)
{
    if (i < 0 || i > GetElementCount())
    {
        return;
    }
    children[i] = elem;
    ((COpticsElement*)&*children[i])->SetParent(this);
}

void COpticsGroup::validateGlobalVars(SAuxParams& aux)
{
    COpticsElement::validateGlobalVars(aux);
    validateChildrenGlobalVars(aux);
}
void COpticsGroup::validateChildrenGlobalVars(SAuxParams& aux)
{
    for (uint i = 0; i < children.size(); i++)
    {
        ((COpticsElement*)GetElementAt(i))->validateGlobalVars(aux);
    }
}

void COpticsGroup::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux)
{
    PROFILE_LABEL_SCOPE("LensEfxGroup");

    for (uint i = 0; i < children.size(); i++)
    {
        if (GetElementAt(i)->IsEnabled())
        {
            ((COpticsElement*)GetElementAt(i))->Render(shader, vSrcWorldPos, vSrcProjPos, aux);
        }
    }
}

void COpticsGroup::GetMemoryUsage(ICrySizer* pSizer) const
{
    for (int i = 0, iChildSize(children.size()); i < iChildSize; ++i)
    {
        children[i]->GetMemoryUsage(pSizer);
    }
    pSizer->AddObject(this, sizeof(*this));
}

void COpticsGroup::Invalidate()
{
    for (int i = 0, iChildSize(children.size()); i < iChildSize; ++i)
    {
        children[i]->Invalidate();
    }
}

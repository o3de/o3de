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

#include "LensFlareElement.h"

// Editor
#include "LensFlareElementTree.h"
#include "LensFlareUtil.h"
#include "LensFlareItem.h"
#include "LensFlareEditor.h"
#include "LensFlareView.h"
#include "LensFlareLibrary.h"


CLensFlareElement::CLensFlareElement()
    : m_vars(NULL)
    , m_pOpticsElement(NULL)
    , m_pParent(NULL)
{
}

CLensFlareElement::~CLensFlareElement()
{
}

void CLensFlareElement::OnInternalVariableChange(IVariable* pVar)
{
    IOpticsElementBasePtr pOptics = GetOpticsElement();
    if (pOptics == NULL)
    {
        return;
    }

    IFuncVariable* pFuncVar = LensFlareUtil::GetFuncVariable(pOptics, pVar->GetUserData().toInt());
    if (pFuncVar == NULL)
    {
        return;
    }

    switch (pVar->GetType())
    {
    case IVariable::INT:
    {
        int var(0);
        pVar->Get(var);
        if (pFuncVar->paramType == e_COLOR)
        {
            ColorF color(pFuncVar->GetColorF());
            color.a = (float)var / 255.0f;
            pFuncVar->InvokeSetter((void*)&color);
        }
        else if (pFuncVar->paramType == e_INT)
        {
            if (LensFlareUtil::HaveParameterLowBoundary(pFuncVar->name.c_str()))
            {
                LensFlareUtil::BoundaryProcess(var);
            }
            pFuncVar->InvokeSetter((void*)&var);
            if (pFuncVar->GetInt() != var)
            {
                pVar->Set(pFuncVar->GetInt());
            }
        }
    }
    break;
    case IVariable::BOOL:
    {
        if (pFuncVar->paramType == e_BOOL)
        {
            bool var;
            pVar->Get(var);
            pFuncVar->InvokeSetter((void*)&var);
        }
    }
    break;
    case IVariable::FLOAT:
    {
        if (pFuncVar->paramType == e_FLOAT)
        {
            float var;
            pVar->Get(var);
            pFuncVar->InvokeSetter((void*)&var);
        }
    }
    break;
    case IVariable::VECTOR2:
    {
        if (pFuncVar->paramType == e_VEC2)
        {
            Vec2 var;
            pVar->Get(var);
            pFuncVar->InvokeSetter((void*)&var);
        }
    }
    break;
    case IVariable::VECTOR:
    {
        Vec3 var;
        pVar->Get(var);
        if (pFuncVar->paramType == e_COLOR)
        {
            ColorF color(pFuncVar->GetColorF());
            color.r = var.x;
            color.g = var.y;
            color.b = var.z;
            pFuncVar->InvokeSetter((void*)&color);
        }
        else if (pFuncVar->paramType == e_VEC3)
        {
            pFuncVar->InvokeSetter((void*)&var);
        }
    }
    break;
    case IVariable::VECTOR4:
    {
        if (pFuncVar->paramType == e_VEC4)
        {
            Vec4 var;
            pVar->Get(var);
            pFuncVar->InvokeSetter((void*)&var);
        }
    }
    break;
    case IVariable::STRING:
    {
        QString var;
        pVar->Get(var);
        if (pFuncVar->paramType == e_TEXTURE2D || pFuncVar->paramType == e_TEXTURE3D || pFuncVar->paramType == e_TEXTURE_CUBE)
        {
            var = var.trimmed();
            ITexture* pTexture = NULL;
            if (!var.isEmpty())
            {
                pTexture = GetIEditor()->GetRenderer()->EF_LoadTexture(var.toUtf8().data());
            }
            pFuncVar->InvokeSetter((void*)pTexture);
            if (pTexture)
            {
                pTexture->Release();
            }
        }
    }
    break;
    }

    UpdateLights();
}

bool CLensFlareElement::IsEnable()
{
    IOpticsElementBasePtr pOptics = GetOpticsElement();
    if (pOptics == NULL)
    {
        return false;
    }
    return pOptics->IsEnabled();
}

void CLensFlareElement::SetEnable(bool bEnable)
{
    IOpticsElementBasePtr pOptics = GetOpticsElement();
    if (pOptics == NULL)
    {
        return;
    }
    pOptics->SetEnabled(bEnable);
    UpdateLights();
}

EFlareType CLensFlareElement::GetOpticsType()
{
    IOpticsElementBasePtr pOptics = GetOpticsElement();
    if (pOptics == NULL)
    {
        return eFT__Base__;
    }
    return pOptics->GetType();
}

bool CLensFlareElement::GetShortName(QString& outName) const
{
    QString fullName;
    if (!GetName(fullName))
    {
        return false;
    }
    int nPos = fullName.lastIndexOf('.');
    if (nPos == -1)
    {
        outName = fullName;
        return true;
    }
    outName = fullName.right(fullName.length() - nPos - 1);
    return true;
}

void CLensFlareElement::UpdateLights()
{
    IOpticsElementBasePtr pOptics = GetOpticsElement();
    if (pOptics == NULL)
    {
        return;
    }
    if (GetLensFlareTree())
    {
        if (GetLensFlareTree()->GetLensFlareItem())
        {
            GetLensFlareTree()->GetLensFlareItem()->UpdateLights(pOptics);
        }
    }
}

void CLensFlareElement::UpdateProperty(IOpticsElementBasePtr pOptics)
{
    std::vector<IVariable::OnSetCallback> funcs;

    if (GetLensFlareTree())
    {
        funcs.push_back(functor(*GetLensFlareTree(), &CLensFlareElementTree::OnInternalVariableChange));
    }

    if (GetLensFlareView())
    {
        funcs.push_back(functor(*GetLensFlareView(), &CLensFlareView::OnInternalVariableChange));
    }

    if (GetLensFlareLibrary())
    {
        funcs.push_back(functor(*GetLensFlareLibrary(), &CLensFlareLibrary::OnInternalVariableChange));
    }

    LensFlareUtil::SetVariablesTemplateFromOptics(pOptics, m_vars, funcs);
}

CLensFlareElementTree* CLensFlareElement::GetLensFlareTree() const
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return NULL;
    }
    return pEditor->GetLensFlareElementTree();
}

CLensFlareView* CLensFlareElement::GetLensFlareView() const
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return NULL;
    }
    return pEditor->GetLensFlareView();
}

CLensFlareLibrary* CLensFlareElement::GetLensFlareLibrary() const
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return NULL;
    }
    return pEditor->GetCurrentLibrary();
}

CLensFlareElement* CLensFlareElement::GetParent() const
{
    return m_pParent;
}

void CLensFlareElement::SetParent(CLensFlareElement* pParent)
{
    m_pParent = pParent;
}

int CLensFlareElement::GetChildCount() const
{
    return m_children.size();
}

CLensFlareElement* CLensFlareElement::GetChildAt(int nPos) const
{
    if (nPos < 0 || nPos >= m_children.size())
    {
        return nullptr;
    }
    return m_children[nPos];
}

void CLensFlareElement::AddChild(CLensFlareElement* pElement)
{
    pElement->SetParent(this);
    m_children.push_back(pElement);
}

void CLensFlareElement::InsertChild(int nPos, CLensFlareElement* pElement)
{
    pElement->SetParent(this);
    m_children.insert(std::begin(m_children) + nPos, pElement);
}

void CLensFlareElement::RemoveChild(int nPos)
{
    m_children.erase(std::begin(m_children) + nPos);
}

void CLensFlareElement::SwapChildren(int nPos1, int nPos2)
{
    std::swap(m_children[nPos1], m_children[nPos2]);
}

void CLensFlareElement::RemoveAllChildren()
{
    m_children.clear();
}

int CLensFlareElement::GetChildIndex(const CLensFlareElement* pElement) const
{
    auto it = std::find_if(
            std::begin(m_children),
            std::end(m_children),
            [=](const LensFlareElementPtr& pChild)
            {
                return pChild.get() == pElement;
            });

    if (it != std::end(m_children))
    {
        return std::distance(std::begin(m_children), it);
    }
    else
    {
        return -1;
    }
}

int CLensFlareElement::GetRow() const
{
    return m_pParent ? m_pParent->GetChildIndex(this) : 0;
}

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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREELEMENT_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREELEMENT_H
#pragma once

#include "IFlares.h"

#include "Util/Variable.h"

class CLensFlareElementTree;
class CLensFlareView;
class CLensFlareLibrary;

class CLensFlareElement
    : public CRefCountBase
{
public:

    typedef _smart_ptr<CLensFlareElement> LensFlareElementPtr;
    typedef std::vector<LensFlareElementPtr> LensFlareElementList;

    CLensFlareElement();
    virtual ~CLensFlareElement();

    CVarBlock* GetProperties() const
    {
        return m_vars;
    }

    void OnInternalVariableChange(IVariable* pVar);

    bool IsEnable();
    void SetEnable(bool bEnable);
    EFlareType GetOpticsType();

    bool GetName(QString& outName) const
    {
        IOpticsElementBasePtr pOptics = GetOpticsElement();
        if (pOptics == NULL)
        {
            return false;
        }
        outName = pOptics->GetName();
        return true;
    }

    bool GetShortName(QString& outName) const;
    IOpticsElementBasePtr GetOpticsElement() const
    {
        return m_pOpticsElement;
    }
    void SetOpticsElement(IOpticsElementBasePtr pOptics)
    {
        m_pOpticsElement = pOptics;
        UpdateProperty(m_pOpticsElement);
    }

    CLensFlareElement* GetParent() const;
    void SetParent(CLensFlareElement* pParent);

    int GetChildCount() const;
    CLensFlareElement* GetChildAt(int nPos) const;

    void AddChild(CLensFlareElement* pElement);
    void InsertChild(int nPos, CLensFlareElement* pElement);

    void RemoveChild(int nPos);
    void RemoveAllChildren();

    void SwapChildren(int nPos1, int nPos2);

    int GetChildIndex(const CLensFlareElement* pChild) const;
    int GetRow() const;

private:

    void UpdateLights();
    void UpdateProperty(IOpticsElementBasePtr pOptics);

    CLensFlareElementTree* GetLensFlareTree() const;
    CLensFlareView* GetLensFlareView() const;
    CLensFlareLibrary* GetLensFlareLibrary() const;

private:

    IOpticsElementBasePtr m_pOpticsElement;
    CVarBlockPtr m_vars;

    CLensFlareElement* m_pParent;
    LensFlareElementList m_children;

    AZStd::map<void*, IVariable::OnSetCallback> m_callbackCache;
};
#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREELEMENT_H

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

#ifndef CRYINCLUDE_EDITOR_ALIGNTOOL_H
#define CRYINCLUDE_EDITOR_ALIGNTOOL_H

#pragma once

//////////////////////////////////////////////////////////////////////////
class CAlignPickCallback
    : public IPickObjectCallback
{
public:
    CAlignPickCallback() { m_bActive = true; };
    //! Called when object picked.
    virtual void OnPick(CBaseObject* picked);
    //! Called when pick mode cancelled.
    virtual void OnCancelPick();
    //! Return true if specified object is pickable.
    virtual bool OnPickFilter(CBaseObject* filterObject);

    static bool IsActive() { return m_bActive; }

    virtual bool IsNeedSpecificBehaviorForSpaceAcce()   {   return true;    }
private:
    static bool m_bActive;
};


#endif // CRYINCLUDE_EDITOR_ALIGNTOOL_H

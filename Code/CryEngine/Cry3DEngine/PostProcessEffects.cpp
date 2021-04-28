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

// Notes : Check PostEffects.h for list of available effects


#include "Cry3DEngine_precompiled.h"
#include "IPostEffectGroup.h"
#include "ITimeOfDay.h"

void C3DEngine::SetPostEffectParam(const char* pParam, float fValue, bool bForceValue) const
{
    if (pParam)
    {
        GetRenderer()->EF_SetPostEffectParam(pParam, fValue, bForceValue);
    }
}

void C3DEngine::GetPostEffectParam(const char* pParam, float& fValue) const
{
    if (pParam)
    {
        GetRenderer()->EF_GetPostEffectParam(pParam, fValue);
    }
}

void C3DEngine::SetPostEffectParamVec4(const char* pParam, const Vec4& pValue, bool bForceValue) const
{
    if (pParam)
    {
        GetRenderer()->EF_SetPostEffectParamVec4(pParam, pValue, bForceValue);
    }
}

void C3DEngine::GetPostEffectParamVec4(const char* pParam, Vec4& pValue) const
{
    if (pParam)
    {
        GetRenderer()->EF_GetPostEffectParamVec4(pParam, pValue);
    }
}

void C3DEngine::SetPostEffectParamString(const char* pParam, const char* pszArg) const
{
    if (pParam && pszArg)
    {
        GetRenderer()->EF_SetPostEffectParamString(pParam, pszArg);
    }
}

void C3DEngine::GetPostEffectParamString(const char* pParam, const char*& pszArg) const
{
    if (pParam)
    {
        GetRenderer()->EF_GetPostEffectParamString(pParam, pszArg);
    }
}

int32 C3DEngine::GetPostEffectID(const char* pPostEffectName)
{
    return GetRenderer()->EF_GetPostEffectID(pPostEffectName);
}

void C3DEngine::ResetPostEffects(bool bOnSpecChange)
{
    GetPostEffectBaseGroup()->ClearParams();
    
    GetRenderer()->EF_ResetPostEffects(bOnSpecChange);

    GetTimeOfDay()->Update(false, true);
}

void C3DEngine::DisablePostEffects()
{
    IPostEffectGroupManager* groupManager = gEnv->p3DEngine->GetPostEffectGroups();
    if (groupManager)
    {
        for (unsigned int i = 0; i < groupManager->GetGroupCount(); i++)
        {
            IPostEffectGroup* group = groupManager->GetGroup(i);
            if (strcmp(group->GetName(), m_defaultPostEffectGroup) != 0 && strcmp(group->GetName(), "Base") != 0)
            {
                group->SetEnable(false);
            }
        }
    }
}

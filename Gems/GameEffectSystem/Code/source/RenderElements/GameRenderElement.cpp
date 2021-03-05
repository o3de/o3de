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
//==================================================================================================
// Name: CGameRenderElement
// Desc: Base class for all game render elements
// Author: James Chilvers
//==================================================================================================

// Includes
#include "GameEffectSystem_precompiled.h"
#include "GameRenderElement.h"

//--------------------------------------------------------------------------------------------------
// Name: CGameRenderElement
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CGameRenderElement::CGameRenderElement()
{
    m_pREGameEffect = NULL;
} //-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: InitialiseGameRenderElement
// Desc: Initialises game render element
//--------------------------------------------------------------------------------------------------
bool CGameRenderElement::InitialiseGameRenderElement()
{
    m_pREGameEffect = (CREGameEffect*)gEnv->pRenderer->EF_CreateRE(eDATA_GameEffect);
    if (m_pREGameEffect)
    {
        m_pREGameEffect->SetPrivateImplementation(this);
        m_pREGameEffect->mfUpdateFlags(FCEF_TRANSFORM);
    }

    return true;
} //-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ReleaseGameRenderElement
// Desc: Releases game render element
//--------------------------------------------------------------------------------------------------
void CGameRenderElement::ReleaseGameRenderElement()
{
    if (m_pREGameEffect)
    {
        m_pREGameEffect->SetPrivateImplementation(NULL);
        m_pREGameEffect->Release(false);
    }
} //-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: UpdatePrivateImplementation
// Desc: Updates private implementation
//--------------------------------------------------------------------------------------------------
void CGameRenderElement::UpdatePrivateImplementation()
{
    if (m_pREGameEffect)
    {
        m_pREGameEffect->SetPrivateImplementation(this);
    }
} //-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetCREGameEffect
// Desc: returns the game effect render element
//--------------------------------------------------------------------------------------------------
CREGameEffect* CGameRenderElement::GetCREGameEffect()
{
    return m_pREGameEffect;
} //-------------------------------------------------------------------------------------------------

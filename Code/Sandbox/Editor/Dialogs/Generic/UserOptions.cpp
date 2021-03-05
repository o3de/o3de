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

// Description : These are helper classes for containing the data from the
//               generic overwrite dialog


#include "EditorDefs.h"

#include "UserOptions.h"

//////////////////////////////////////////////////////////////////////////
CUserOptions::CUserOptionsReferenceCountHelper::CUserOptionsReferenceCountHelper(CUserOptions& roUserOptions)
    : m_roReferencedUserOptionsObject(roUserOptions)
{
    m_roReferencedUserOptionsObject.IncRef();
}
//////////////////////////////////////////////////////////////////////////
CUserOptions::CUserOptionsReferenceCountHelper::~CUserOptionsReferenceCountHelper()
{
    m_roReferencedUserOptionsObject.DecRef();
}
//////////////////////////////////////////////////////////////////////////
CUserOptions::CUserOptions()
{
    m_boToAll = false;
    m_nCurrentOption = ENotSet;
}
//////////////////////////////////////////////////////////////////////////
bool    CUserOptions::IsOptionValid()
{
    return m_nCurrentOption != ENotSet;
}
//////////////////////////////////////////////////////////////////////////
int CUserOptions::GetOption()
{
    return m_nCurrentOption;
}
//////////////////////////////////////////////////////////////////////////
bool    CUserOptions::IsOptionToAll()
{
    return m_boToAll;
}
//////////////////////////////////////////////////////////////////////////
void    CUserOptions::SetOption(int nNewOption, bool boToAll)
{
    m_nCurrentOption = nNewOption;
    m_boToAll = boToAll;
}
//////////////////////////////////////////////////////////////////////////
int CUserOptions::DecRef()
{
    if (m_nNumberOfReferences >= 1)
    {
        --m_nNumberOfReferences;
        if (m_nNumberOfReferences == 0)
        {
            SetOption(CUserOptions::ENotSet, false);
        }
    }
    return m_nNumberOfReferences;
}
//////////////////////////////////////////////////////////////////////////
int CUserOptions::IncRef()
{
    ++m_nNumberOfReferences;
    return m_nNumberOfReferences;
}
//////////////////////////////////////////////////////////////////////////

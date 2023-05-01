/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Used in a property item to select a light animation


#include "EditorDefs.h"

#include "SelectLightAnimationDialog.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>   // for AnimNodeType

//////////////////////////////////////////////////////////////////////////
CSelectLightAnimationDialog::CSelectLightAnimationDialog(QWidget* pParent)
    :    CGenericSelectItemDialog(pParent)
{
    setWindowTitle(tr("Select Light Animation"));
}

//////////////////////////////////////////////////////////////////////////
void CSelectLightAnimationDialog::OnInitDialog()
{
    SetMode(eMODE_LIST);
    CGenericSelectItemDialog::OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
void CSelectLightAnimationDialog::GetItems(std::vector<SItem>& outItems)
{
    IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
    if (pMovieSystem)
    {
        for (int i = 0; i < pMovieSystem->GetNumSequences(); ++i)
        {
            IAnimSequence* pSequence = pMovieSystem->GetSequence(i);
            if ((pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet) == 0)
            {
                continue;
            }

            for (int k = 0; k < pSequence->GetNodeCount(); ++k)
            {
                assert(pSequence->GetNode(k)->GetType() == AnimNodeType::Light);
                if (pSequence->GetNode(k)->GetType() != AnimNodeType::Light)
                {
                    continue;
                }
                SItem item;
                item.name = pSequence->GetNode(k)->GetName();
                outItems.push_back(item);
            }
            return;
        }
    }
}

#include <moc_SelectLightAnimationDialog.cpp>

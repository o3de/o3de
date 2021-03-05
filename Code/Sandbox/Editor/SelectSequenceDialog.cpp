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

#include "SelectSequenceDialog.h"

// CSelectSequence dialog

//////////////////////////////////////////////////////////////////////////
CSelectSequenceDialog::CSelectSequenceDialog(QWidget* pParent)
    :    CGenericSelectItemDialog(pParent)
{
    setWindowTitle(tr("Select Sequence"));
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void
CSelectSequenceDialog::OnInitDialog()
{
    SetMode(eMODE_LIST);
    CGenericSelectItemDialog::OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void
CSelectSequenceDialog::GetItems(std::vector<SItem>& outItems)
{
    IMovieSystem* pMovieSys = GetIEditor()->GetMovieSystem();
    for (int i = 0; i < pMovieSys->GetNumSequences(); ++i)
    {
        IAnimSequence* pSeq = pMovieSys->GetSequence(i);
        SItem item;
        string fullname = pSeq->GetName();
        item.name = fullname.c_str();
        outItems.push_back(item);
    }
}

#include <moc_SelectSequenceDialog.cpp>

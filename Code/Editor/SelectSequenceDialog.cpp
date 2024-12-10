/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        for (int i = 0; i < movieSystem->GetNumSequences(); ++i)
        {
            IAnimSequence* pSeq = movieSystem->GetSequence(i);
            SItem item;
            AZStd::string fullname = pSeq->GetName();
            item.name = fullname.c_str();
            outItems.push_back(item);
        }
    }
}

#include <moc_SelectSequenceDialog.cpp>

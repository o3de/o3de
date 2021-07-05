/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "DimensionsDialog.h"

// Qt
#include <QButtonGroup>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_DimensionsDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING



/////////////////////////////////////////////////////////////////////////////
CDimensionsDialog::CDimensionsDialog(QWidget* pParent /*=nullptr*/)
    : QDialog(pParent)
    , m_group(new QButtonGroup(this))
    , ui(new Ui::CDimensionsDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Generate Terrain Texture"));

    m_group->addButton(ui->Dim512, 512);
    m_group->addButton(ui->Dim1024, 1024);
    m_group->addButton(ui->Dim2048, 2048);
    m_group->addButton(ui->Dim4096, 4096);
    m_group->addButton(ui->Dim8192, 8192);
    m_group->addButton(ui->Dim16384, 16384);
}


//////////////////////////////////////////////////////////////////////////
CDimensionsDialog::~CDimensionsDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CDimensionsDialog::SetDimensions(unsigned int iWidth)
{
    ////////////////////////////////////////////////////////////////////////
    // Select a dimension option button in the dialog
    ////////////////////////////////////////////////////////////////////////

    QAbstractButton* button = m_group->button(iWidth);
    assert(button);

    button->setChecked(true);
}

UINT CDimensionsDialog::GetDimensions()
{
    ////////////////////////////////////////////////////////////////////////
    // Get the currently selected dimension option button in the dialog
    ////////////////////////////////////////////////////////////////////////

    assert(m_group->checkedId() != -1);

    return m_group->checkedId();
}

#include <moc_DimensionsDialog.cpp>

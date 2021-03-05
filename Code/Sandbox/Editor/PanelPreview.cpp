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

#include "PanelPreview.h"

// Qt
#include <QBoxLayout>

// CPanelPreview dialog

CPanelPreview::CPanelPreview(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , m_previewCtrl(new CPreviewModelCtrl(this))
{
    QBoxLayout* layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_previewCtrl);
    setLayout(layout);
}

//////////////////////////////////////////////////////////////////////////
void CPanelPreview::LoadFile(const QString& filename)
{
    if (!filename.isEmpty())
    {
        m_previewCtrl->EnableUpdate(false);
        m_previewCtrl->LoadFile(filename, false);
    }
}


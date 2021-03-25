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

#include "EnvironmentPanel.h"

// Editor
#include "GameEngine.h"
#include "CryEditDoc.h"

// Cry3DEngine
#include <Cry3DEngine/Environment/OceanEnvironmentBus.h>


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_EnvironmentPanel.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

/////////////////////////////////////////////////////////////////////////////
// CEnvironmentPanel dialog

CEnvironmentPanel::CEnvironmentPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CEnvironmentPanel)
{
    XmlNodeRef node = GetIEditor()->GetDocument()->GetEnvironmentTemplate();

    // is the feature toggle enabled?
    bool bHasOceanFeature = false;
    AZ::OceanFeatureToggleBus::BroadcastResult(bHasOceanFeature, &AZ::OceanFeatureToggleBus::Events::OceanComponentEnabled);
    if (bHasOceanFeature)
    {
        node->findChild("Ocean")->setAttr("hidden", true);
        node->findChild("OceanAnimation")->setAttr("hidden", true);
    }

    m_onSetCallback = AZStd::bind(&CCryEditDoc::OnEnvironmentPropertyChanged, GetIEditor()->GetDocument(), AZStd::placeholders::_1);

    ui->setupUi(this);
    ui->m_wndProps->Setup();
    ui->m_wndProps->CreateItems(node, m_varBlock, &m_onSetCallback, true);
    ui->m_wndProps->RebuildCtrl(false);
    ui->m_wndProps->ExpandAll();
    connect(ui->APPLYBTN, &QPushButton::clicked, this, &CEnvironmentPanel::OnBnClickedApply);
}

CEnvironmentPanel::~CEnvironmentPanel()
{
}

//////////////////////////////////////////////////////////////////////////
void CEnvironmentPanel::OnBnClickedApply()
{
    GetIEditor()->GetGameEngine()->ReloadEnvironment();
}

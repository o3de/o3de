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

#include "LensFlareView.h"

// Editor
#include "LensFlareItem.h"
#include "LensFlareEditor.h"
#include "Objects/EntityObject.h"
#include "Material/MaterialManager.h"


CLensFlareView::CLensFlareView(QWidget* parent, Qt::WindowFlags f)
    : CPreviewModelCtrl(parent, f)
{
    InitDialog();
}

CLensFlareView::~CLensFlareView()
{
    CLensFlareEditor::GetLensFlareEditor()->UnregisterLensFlareItemChangeListener(this);
}

void CLensFlareView::InitDialog()
{
    CLensFlareEditor::GetLensFlareEditor()->RegisterLensFlareItemChangeListener(this);

    SetClearColor(ColorF(0, 0, 0, 0));
    SetGrid(true);
    SetAxis(true);
    EnableUpdate(true);
    m_pLensFlareMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(CEntityObject::s_LensFlareMaterialName);
    m_InitCameraPos = m_camera.GetPosition();
}

void CLensFlareView::RenderObject(_smart_ptr<IMaterial> pMaterial, [[maybe_unused]] SRenderingPassInfo& passInfo)
{
    if (m_pLensFlareItem)
    {
        IOpticsElementBasePtr pOptics = m_pLensFlareItem->GetOptics();
        if (pOptics && m_pLensFlareMaterial)
        {
            _smart_ptr<IMaterial> pMaterial = m_pLensFlareMaterial->GetMatInfo();
            if (pMaterial)
            {
                m_pRenderer->ForceUpdateGlobalShaderParameters();
                m_pRenderer->SetViewport(1, 1, m_pRenderer->GetWidth(), m_pRenderer->GetHeight());
                SShaderItem& shaderItem = pMaterial->GetShaderItem();
                SLensFlareRenderParam param;
                param.pCamera = &m_camera;
                param.pShader = shaderItem.m_pShader;
                pOptics->Render(&param, Vec3(0, 0, 0));
            }
        }
    }
}

void CLensFlareView::OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem)
{
    if (m_pLensFlareItem != pLensFlareItem)
    {
        m_pLensFlareItem = pLensFlareItem;
        Update(true);
        m_bHaveAnythingToRender = m_pLensFlareItem != NULL;
    }
}

void CLensFlareView::OnLensFlareDeleteItem(CLensFlareItem* pLensFlareItem)
{
    if (m_pLensFlareItem == pLensFlareItem)
    {
        m_pLensFlareItem = NULL;
        Update();
        m_bHaveAnythingToRender = false;
    }
}

void CLensFlareView::OnInternalVariableChange([[maybe_unused]] IVariable* pVar)
{
    Update();
}

void CLensFlareView::OnLensFlareChangeElement([[maybe_unused]] CLensFlareElement* pLensFlareElement)
{
    Update();
}

void CLensFlareView::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnCloseScene:
        ReleaseObject();
        break;
    }
}

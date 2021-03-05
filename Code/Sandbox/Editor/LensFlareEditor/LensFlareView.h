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

#ifndef CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREVIEW_H
#define CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREVIEW_H
#pragma once

#include "ILensFlareListener.h"
#include "Controls/PreviewModelCtrl.h"

class CLensFlareView
    : public CPreviewModelCtrl
    , public ILensFlareChangeItemListener
    , public ILensFlareChangeElementListener
{
public:
    explicit CLensFlareView(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~CLensFlareView();

    void OnInternalVariableChange(IVariable* pVar);
    void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:

    void InitDialog();
    void ProcessKeys(){}
    void RenderObject(_smart_ptr<IMaterial> pMaterial, SRenderingPassInfo& passInfo);

    void OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem);
    void OnLensFlareDeleteItem(CLensFlareItem* pLensFlareItem);
    void OnLensFlareChangeElement(CLensFlareElement* pLensFlareElement);

private:

    QPoint m_prevPoint;
    Vec3 m_InitCameraPos;

    _smart_ptr<CMaterial> m_pLensFlareMaterial;
    _smart_ptr<CLensFlareItem> m_pLensFlareItem;
};
#endif // CRYINCLUDE_EDITOR_LENSFLAREEDITOR_LENSFLAREVIEW_H

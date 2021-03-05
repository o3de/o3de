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

#ifndef CRYINCLUDE_EDITOR_PANELPREVIEW_H
#define CRYINCLUDE_EDITOR_PANELPREVIEW_H
#pragma once

// CPanelPreview dialog

#include "Controls/PreviewModelCtrl.h"


class CPanelPreview
    : public QWidget
{
public:
    CPanelPreview(QWidget* pParent = nullptr);   // standard constructor

    void LoadFile(const QString& filename);

    QSize sizeHint() const override
    {
        return QSize(130, 240);
    }

protected:
    CPreviewModelCtrl* m_previewCtrl;
};

#endif // CRYINCLUDE_EDITOR_PANELPREVIEW_H

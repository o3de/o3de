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
#pragma once

#include "PreviewModelView.h"

class MaterialPreviewModelView
    : public CPreviewModelView
{
public:
    // set enableIdleUpdate to false if you don't want the view to update itself during
    // application idle notification (and resize events). That makes sense when this view
    // is only used to render into memory bitmaps. Note that the view has to been visible
    // for that, but can be somewhere of-screen.
    MaterialPreviewModelView(QWidget* parent, bool enableIdleUpdate = true);
    virtual ~MaterialPreviewModelView();
    void SetCameraLookAt(float fRadiusScale, const Vec3& fromDir);
    void SetMaterial(_smart_ptr<IMaterial> material);
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    bool m_enableIdleUpdate;
};

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

#include "EditorDefs.h"

#include "MaterialPreviewModelView.h"



MaterialPreviewModelView::MaterialPreviewModelView(QWidget* parent, bool enableIdleUpdate)
    : CPreviewModelView(parent)
    , m_enableIdleUpdate(enableIdleUpdate)
{}

MaterialPreviewModelView::~MaterialPreviewModelView()
{}

void MaterialPreviewModelView::SetCameraLookAt(float fRadiusScale, const Vec3& fromDir)
{
    IStatObj* target = GetStaticModel();
    if (target)
    {
        Vec3 dir = fromDir.GetNormalized();

        // Calculate the camera matrix
        Matrix34 tm = Matrix33::CreateRotationVDir(dir, 0);
        Vec3 translation = target->GetAABB().GetCenter() - dir * target->GetAABB().GetRadius() * fRadiusScale;
        tm.SetTranslation(translation);

        // Convert it to a quaternion and move the viewport's camera
        QuatT quat(tm);
        CameraMoved(quat, true);
    }
}

void MaterialPreviewModelView::SetMaterial(_smart_ptr<IMaterial> material)
{
    IStatObj* staticModel = GetStaticModel();
    if (staticModel)
    {
        staticModel->SetMaterial(material);
    }
}

void MaterialPreviewModelView::resizeEvent(QResizeEvent* event)
{
    if (m_enableIdleUpdate)
    {
        CPreviewModelView::resizeEvent(event);
    }
}

void MaterialPreviewModelView::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnIdleUpdate:
    {
        if (m_enableIdleUpdate)
        {
            Update();
        }
        break;
    }
    default:
        break;
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Undo for Python function (PySetCurrentViewPosition)


#include "EditorDefs.h"

#include "UndoViewRotation.h"

// Editor
#include "ViewManager.h"

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzFramework/Components/CameraBus.h>
#include <MathConversion.h>

Ang3 CUndoViewRotation::GetActiveCameraRotation()
{
    AZ::Transform activeCameraTm = AZ::Transform::CreateIdentity();
    Camera::ActiveCameraRequestBus::BroadcastResult(
        activeCameraTm,
        &Camera::ActiveCameraRequestBus::Events::GetActiveCameraTransform
    );
    const AZ::Matrix3x4 cameraMatrix = AZ::Matrix3x4::CreateFromTransform(activeCameraTm);
    const Matrix33 cameraMatrixCry = AZMatrix3x3ToLYMatrix3x3(AZ::Matrix3x3::CreateFromMatrix3x4(cameraMatrix));
    return RAD2DEG(Ang3::GetAnglesXYZ(cameraMatrixCry));
}

CUndoViewRotation::CUndoViewRotation(const QString& pUndoDescription)
{
    m_undoDescription = pUndoDescription;
    m_undo = GetActiveCameraRotation();
}

int CUndoViewRotation::GetSize()
{
    return sizeof(*this);
}

QString CUndoViewRotation::GetDescription()
{
    return m_undoDescription;
}

void CUndoViewRotation::Undo(bool bUndo)
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        if (bUndo)
        {
            m_redo = GetActiveCameraRotation();
        }

        Matrix34 tm = pRenderViewport->GetViewTM();
        tm.SetRotationXYZ(Ang3(DEG2RAD(m_undo.x), DEG2RAD(m_undo.y), DEG2RAD(m_undo.z)), tm.GetTranslation());
        pRenderViewport->SetViewTM(tm);
    }
}

void CUndoViewRotation::Redo()
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();
        tm.SetRotationXYZ(Ang3(DEG2RAD(m_redo.x), DEG2RAD(m_redo.y), DEG2RAD(m_redo.z)), tm.GetTranslation());
        pRenderViewport->SetViewTM(tm);
    }
}

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

#include "AlignTool.h"

// Editor
#include "Objects/BaseObject.h"
#include "Objects/SelectionGroup.h"

//////////////////////////////////////////////////////////////////////////
bool CAlignPickCallback::m_bActive = false;

//////////////////////////////////////////////////////////////////////////
//! Called when object picked.
void CAlignPickCallback::OnPick(CBaseObject* picked)
{
    Matrix34 pickedTM(picked->GetWorldTM());

    AABB pickedAABB;
    picked->GetBoundBox(pickedAABB);
    pickedAABB.Move(-pickedTM.GetTranslation());
    Vec3 pickedPivot = pickedAABB.GetCenter();

    AABB pickedLocalAABB;
    picked->GetLocalBounds(pickedLocalAABB);

    const Quat& pickedRot = picked->GetRotation();
    const Vec3& pickedScale = picked->GetScale();
    const Vec3& pickedPos = picked->GetPos();

    bool bKeepScale = CheckVirtualKey(Qt::Key_Shift);
    bool bKeepRotation = CheckVirtualKey(Qt::Key_Alt);
    bool bAlignToBoundBox = CheckVirtualKey(Qt::Key_Control);
    bool bApplyTransform = !bKeepScale && !bKeepRotation && !bAlignToBoundBox;

    {
        bool bUndo = !CUndo::IsRecording();
        if (bUndo)
        {
            GetIEditor()->BeginUndo();
        }

        CSelectionGroup* selGroup = GetIEditor()->GetSelection();
        selGroup->FilterParents();

        for (int i = 0; i < selGroup->GetFilteredCount(); i++)
        {
            CBaseObject* pMovedObj = selGroup->GetFilteredObject(i);

            if (bKeepScale || bKeepRotation || bApplyTransform)
            {
                if (bKeepScale && bKeepRotation)  // Keep scale and rotation of a moved object
                {
                    pMovedObj->SetWorldTM(Matrix34::Create(pMovedObj->GetScale(), pMovedObj->GetRotation(), pickedPos), eObjectUpdateFlags_UserInput);
                }
                else if (bKeepScale)  // Keep only scale of a moved object
                {
                    pMovedObj->SetWorldTM(Matrix34::Create(pMovedObj->GetScale(), pickedRot, pickedPos), eObjectUpdateFlags_UserInput);
                }
                else if (bKeepRotation)   // Keep only rotation of a moved object
                {
                    pMovedObj->SetWorldTM(Matrix34::Create(pickedScale, pMovedObj->GetRotation(), pickedPos), eObjectUpdateFlags_UserInput);
                }
                else // Scale, Rotation and Position of a picked object are applied to a moved object.
                {
                    pMovedObj->SetWorldTM(pickedTM, eObjectUpdateFlags_UserInput);
                }
            }
            else if (bAlignToBoundBox)  // align to the bounding box.
            {
                if (pickedLocalAABB.GetVolume() == 0.0f)
                {
                    continue;
                }

                AABB movedLocalAABB;
                pMovedObj->GetLocalBounds(movedLocalAABB);
                if (fabs(movedLocalAABB.max.x - movedLocalAABB.min.x) < VEC_EPSILON &&
                    fabs(movedLocalAABB.max.y - movedLocalAABB.min.y) < VEC_EPSILON &&
                    fabs(movedLocalAABB.max.z - movedLocalAABB.min.z) < VEC_EPSILON)
                {
                    continue;
                }

                const Vec3& movedScale(pMovedObj->GetScale());
                Matrix34 movedScaleTM = Matrix34::CreateScale(movedScale);
                AABB movedLocalScaledAABB;
                movedLocalScaledAABB.min = movedScaleTM.TransformVector(movedLocalAABB.min);
                movedLocalScaledAABB.max = movedScaleTM.TransformVector(movedLocalAABB.max);

                float fMovedWidth = movedLocalScaledAABB.max.x - movedLocalScaledAABB.min.x;
                float fMovedHeight = movedLocalScaledAABB.max.z - movedLocalScaledAABB.min.z;
                float fMovedLength = movedLocalScaledAABB.max.y - movedLocalScaledAABB.min.y;

                Matrix34 pickedScaleTM = Matrix34::CreateScale(picked->GetScale());
                AABB pickedLocalScaledAABB;
                pickedLocalScaledAABB.min = pickedScaleTM.TransformVector(pickedLocalAABB.min);
                pickedLocalScaledAABB.max = pickedScaleTM.TransformVector(pickedLocalAABB.max);
                float fScaledPickedtWidth = pickedLocalScaledAABB.max.x - pickedLocalScaledAABB.min.x;
                float fScaledPickedHeight = pickedLocalScaledAABB.max.z - pickedLocalScaledAABB.min.z;
                float fScaledPickedLength = pickedLocalScaledAABB.max.y - pickedLocalScaledAABB.min.y;

                Vec3 scale((fScaledPickedtWidth / fMovedWidth) * movedScale.x, (fScaledPickedLength / fMovedLength) * movedScale.y, (fScaledPickedHeight / fMovedHeight) * movedScale.z);
                Matrix34 scaleRotTM = Matrix34::Create(scale, pickedRot, Vec3(0, 0, 0));
                Vec3 movedPivot = scaleRotTM.TransformVector(movedLocalAABB.GetCenter());

                pMovedObj->SetWorldTM(Matrix34::Create(scale, pickedRot, Vec3(pickedPos + (pickedPivot - movedPivot))), eObjectUpdateFlags_UserInput);
            }
        }
        m_bActive = false;
        if (bUndo)
        {
            GetIEditor()->AcceptUndo("Align To Object");
        }
    }
    delete this;
}

//! Called when pick mode cancelled.
void CAlignPickCallback::OnCancelPick()
{
    m_bActive = false;
    delete this;
}

//! Return true if specified object is pickable.
bool CAlignPickCallback::OnPickFilter([[maybe_unused]] CBaseObject* filterObject)
{
    return true;
};

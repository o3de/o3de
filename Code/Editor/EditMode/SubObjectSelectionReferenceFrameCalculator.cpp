/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Calculate the reference frame for sub-object selections.

#include "EditorDefs.h"

#include "SubObjectSelectionReferenceFrameCalculator.h"

SubObjectSelectionReferenceFrameCalculator::SubObjectSelectionReferenceFrameCalculator(ESubObjElementType selectionType)
    :   m_anySelected(false)
    , pos(0.0f, 0.0f, 0.0f)
    , normal(0.0f, 0.0f, 0.0f)
    , nNormals(0)
    , selectionType(selectionType)
    , bUseExplicitFrame(false)
    , bExplicitAnySelected(false)
{
}

void SubObjectSelectionReferenceFrameCalculator::SetExplicitFrame(bool bAnySelected, const Matrix34& refFrame)
{
    this->m_refFrame = refFrame;
    this->bUseExplicitFrame = true;
    this->bExplicitAnySelected = bAnySelected;
}

bool SubObjectSelectionReferenceFrameCalculator::GetFrame(Matrix34& refFrame)
{
    if (this->bUseExplicitFrame)
    {
        refFrame = this->m_refFrame;
        return this->bExplicitAnySelected;
    }
    else
    {
        refFrame.SetIdentity();

        if (this->nNormals > 0)
        {
            this->normal = this->normal / this->nNormals;
            if (!this->normal.IsZero())
            {
                this->normal.Normalize();
            }

            // Average position.
            this->pos = this->pos / this->nNormals;
            refFrame.SetTranslation(this->pos);
        }

        if (this->m_anySelected)
        {
            if (!this->normal.IsZero())
            {
                Vec3 xAxis(1, 0, 0), yAxis(0, 1, 0), zAxis(0, 0, 1);
                if (this->normal.IsEquivalent(zAxis) || normal.IsEquivalent(-zAxis))
                {
                    zAxis = xAxis;
                }
                xAxis = this->normal.Cross(zAxis).GetNormalized();
                yAxis = xAxis.Cross(this->normal).GetNormalized();
                refFrame.SetFromVectors(xAxis, yAxis, normal, pos);
            }
        }

        return m_anySelected;
    }
}

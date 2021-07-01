/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Calculate the reference frame for sub-object selections.


#ifndef CRYINCLUDE_EDITOR_EDITMODE_SUBOBJECTSELECTIONREFERENCEFRAMECALCULATOR_H
#define CRYINCLUDE_EDITOR_EDITMODE_SUBOBJECTSELECTIONREFERENCEFRAMECALCULATOR_H
#pragma once


#include "ISubObjectSelectionReferenceFrameCalculator.h"
#include "Objects/SubObjSelection.h"

class SubObjectSelectionReferenceFrameCalculator
    : public ISubObjectSelectionReferenceFrameCalculator
{
public:
    SubObjectSelectionReferenceFrameCalculator(ESubObjElementType selectionType);

    virtual void SetExplicitFrame(bool bAnySelected, const Matrix34& refFrame);
    bool GetFrame(Matrix34& refFrame);

private:
    bool m_anySelected;
    Vec3 pos;
    Vec3 normal;
    int nNormals;
    ESubObjElementType selectionType;
    std::vector<Vec3> positions;
    Matrix34 m_refFrame;
    bool bUseExplicitFrame;
    bool bExplicitAnySelected;
};

#endif // CRYINCLUDE_EDITOR_EDITMODE_SUBOBJECTSELECTIONREFERENCEFRAMECALCULATOR_H

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

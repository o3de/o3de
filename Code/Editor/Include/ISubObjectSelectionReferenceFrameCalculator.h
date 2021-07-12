/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Calculate the reference frame for sub-object selections.


#ifndef CRYINCLUDE_EDITOR_INCLUDE_ISUBOBJECTSELECTIONREFERENCEFRAMECALCULATOR_H
#define CRYINCLUDE_EDITOR_INCLUDE_ISUBOBJECTSELECTIONREFERENCEFRAMECALCULATOR_H
#pragma once


class ISubObjectSelectionReferenceFrameCalculator
{
public:
    virtual void SetExplicitFrame(bool bAnySelected, const Matrix34& refFrame) = 0;
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_ISUBOBJECTSELECTIONREFERENCEFRAMECALCULATOR_H

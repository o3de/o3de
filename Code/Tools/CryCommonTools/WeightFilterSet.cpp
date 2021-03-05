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

#include <platform.h>
#include <assert.h>                                     // assert()
#include "WeightFilterSet.h"                    // CWeightFilterSet



void CWeightFilterSet::FreeData()
{
    m_FilterKernelBlock.FreeData();
}


bool CWeightFilterSet::Create(const unsigned long indwSideLength, const CSummedAreaFilterKernel& inFilter, const float infR)
{
    assert(indwSideLength >= 1);

    FreeData();

    // 32 Baustelle
    inFilter.CreateWeightFilterBlock(m_FilterKernelBlock, 1, infR * indwSideLength);
    return(true);
}


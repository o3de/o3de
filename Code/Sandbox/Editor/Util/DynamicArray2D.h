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

// Description : Interface of the class CDynamicArray.


#ifndef CRYINCLUDE_EDITOR_UTIL_DYNAMICARRAY2D_H
#define CRYINCLUDE_EDITOR_UTIL_DYNAMICARRAY2D_H
#pragma once


class CDynamicArray2D
{
public:
    // constructor
    CDynamicArray2D(unsigned int iDimension1, unsigned int iDimension2);
    // destructor
    virtual ~CDynamicArray2D();
    //
    void ScaleImage(CDynamicArray2D* pDestination);
    //
    void GetMemoryUsage(ICrySizer* pSizer);


    float**                    m_Array;                     //

private:

    unsigned int            m_Dimension1;           //
    unsigned int            m_Dimension2;           //
};

#endif // CRYINCLUDE_EDITOR_UTIL_DYNAMICARRAY2D_H

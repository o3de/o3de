/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

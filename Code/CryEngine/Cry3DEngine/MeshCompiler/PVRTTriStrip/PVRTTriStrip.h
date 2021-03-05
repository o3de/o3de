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
/*!****************************************************************************

 @file         PVRTTriStrip.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Strips a triangle list.

******************************************************************************/
#ifndef _PVRTTRISTRIP_H_
#define _PVRTTRISTRIP_H_


/****************************************************************************
** Declarations
****************************************************************************/

/*!***************************************************************************
 @brief             Reads a triangle list and generates an optimised triangle strip.
 @param[out]        ppui32Strips
 @param[out]        ppnStripLen
 @param[out]        pnStripCnt
 @param[in]         pui32TriList
 @param[in]         nTriCnt
*****************************************************************************/
void PVRTTriStrip(
    unsigned int** ppui32Strips,
    unsigned int** ppnStripLen,
    unsigned int* pnStripCnt,
    const unsigned int* const pui32TriList,
    const unsigned int      nTriCnt);


/*!***************************************************************************
 @brief             Reads a triangle list and generates an optimised triangle strip. Result is
                    converted back to a triangle list.
 @param[in,out]     pui32TriList
 @param[in]         nTriCnt
*****************************************************************************/
void PVRTTriStripList(unsigned int* const pui32TriList, const unsigned int nTriCnt);


#endif /* _PVRTTRISTRIP_H_ */

/*****************************************************************************
 End of file (PVRTTriStrip.h)
*****************************************************************************/


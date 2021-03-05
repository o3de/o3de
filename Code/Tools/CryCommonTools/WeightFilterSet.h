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

#pragma once
#ifndef CRYINCLUDE_CRYCOMMONTOOLS_WEIGHTFILTERSET_H
#define CRYINCLUDE_CRYCOMMONTOOLS_WEIGHTFILTERSET_H


#include "SimpleBitmap.h"                           // SimpleBitmap<>
#include <vector>                                           // STL vector<>
#include "SummedAreaFilterKernel.h"     // CSummedAreaFilterKernel

class CWeightFilterSet
{
public:

    //! /param indwSideLength [1,..[ e.g. 3 for 3x3 block
    bool Create(const unsigned long indwSideLength, const CSummedAreaFilterKernel& inFilter, const float infR);

    //!
    void FreeData();

    //! optimizable
    //! weight is 1.0
    //! /param iniX x position in inoutDest
    //! /param iniY y position in inoutDest
    //! /param TInputImage typically CSimpleBitmap<TElement>
    //! /return weight
    template <class TElement, class TInputImage >
    float GetBlockWithFilter(const TInputImage& inSrc, const int iniX, const int iniY, TElement& outResult)
    {
        float fWeightSum = 0.0f;
        CSimpleBitmap<float>& rBitmap = m_FilterKernelBlock;

        int W = (int)rBitmap.GetWidth();
        int H = (int)rBitmap.GetHeight();

        int iSrcW = (int)inSrc.GetWidth();
        int iSrcH = (int)inSrc.GetHeight();

        float* pfWeights = rBitmap.GetPointer(0, 0);

        for (int y = 0; y < H; y++)
        {
            int iDestY = y + iniY - H / 2;

            // optimizable (don't use the bottom border)
            //          if(iDestY==iSrcH){ pfWeights+=H;continue; }

            for (int x = 0; x < W; x++, pfWeights++)
            {
                int iDestX = x + iniX - W / 2;

                // optimizable (don't use the right border)
                //              if(iDestX==iSrcW)
                //                  continue;

                TElement Value;

                //              if(inSrc.Get(iDestX,iDestY,Value))
                if (inSrc.Get((iDestX + iSrcW * 2) % iSrcW, (iDestY + iSrcH * 2) % iSrcH, Value))          // tiled
                {
                    float fWeight = *pfWeights;

                    outResult += Value * fWeight;
                    fWeightSum += fWeight;
                }
            }
        }

        return fWeightSum;
    }

    int GetBorderSize()
    {
        int W = (int)m_FilterKernelBlock.GetWidth();

        return (W - 1) / 2;
    }

private: // -------------------------------------------------------------

    CSimpleBitmap<float>                                        m_FilterKernelBlock;            //!< weight = 1
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_WEIGHTFILTERSET_H

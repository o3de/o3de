/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "FIR-Windows.h"

namespace ImageProcessingAtom
{
    /* ####################################################################################################################
     */
    template<class DataType>
    inline DataType abs    (const DataType& ths) { return (ths < 0 ? -ths : ths); }
    template<class DataType>
    inline void     minmax (const DataType& ths, DataType& mn, DataType& mx) { mn = (mn > ths ? ths : mn); mx = (mx < ths ? ths : mx); }
    template<class DataType>
    inline DataType minimum(const DataType& ths, const DataType& tht) { return (ths < tht ? ths : tht); }
    template<class DataType>
    inline DataType maximum(const DataType& ths, const DataType& tht) { return (ths > tht ? ths : tht); }

    #ifndef round
    #define round(x)    ((x) >= 0) ? floor((x) + 0.5) : ceil((x) - 0.5)
    #endif

    /* ####################################################################################################################
     */

    template<class T>
    class FilterWeights
    {
    public:
        FilterWeights()
            : weights(nullptr)
        {
        }

        ~FilterWeights()
        {
            delete[] weights;
        }

    public:
        // window-position
        int first, last;

        // do we encounter positive as well as negative weights
        bool hasNegativeWeights;

        /* weights, summing up to -(1 << 15),
         * means weights are given negative
         * that enables us to use signed short
         * multiplication while occupying 0x8000
         */
        T* weights;
    };

    /* ####################################################################################################################
     */

    void               calculateFilterRange (unsigned int srcFactor, int& srcFirst, int& srcLast,
        unsigned int dstFactor, int  dstFirst, int  dstLast,
        double blurFactor, class IWindowFunction<double>* windowFunction);

    template<typename T>
    FilterWeights<T>* calculateFilterWeights(unsigned int srcFactor, int  srcFirst, int  srcLast,
        unsigned int dstFactor, int  dstFirst, int  dstLast, signed short int numRepetitions,
        double blurFactor, class IWindowFunction<double>* windowFunction,
            bool peaknorm, bool& plusminus);

    template<>
    FilterWeights<signed short int>* calculateFilterWeights<signed short int>(unsigned int srcFactor, int srcFirst, int srcLast,
        unsigned int dstFactor, int dstFirst, int dstLast, signed short int numRepetitions,
        double blurFactor, class IWindowFunction<double>* windowFunction,
        bool peaknorm, bool& plusminus);
} //end namespace ImageProcessingAtom

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "DataInterface.h"

namespace GraphCanvas
{
    class NumericDataInterface 
        : public DataInterface
    {
    public:    
        virtual double GetNumber() const = 0;
        virtual void SetNumber(double value) = 0;

        virtual int GetDecimalPlaces() const
        {
            return 7;
        }

        virtual int GetDisplayDecimalPlaces() const
        {
            return 4;
        }
        
        virtual double GetMin() const
        {
            // Had some weird issues with the numeric limits min
            // and the QtDoubleSpinBox not accepting it(would turn it into a zero)
            // And adding 1 to the numeric_limits<double>::min() turned it into a 1.
            // So, hardcoding to some sufficiently large negative value.
            return -999999999;
        }

        virtual double GetMax() const
        {
            return std::numeric_limits<double>::max();
        }

        const char* GetSuffix() const
        {
            return "";
        }
    };
}

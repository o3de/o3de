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
#pragma once

#include "DataInterface.h"
#include "NumericDataInterface.h"

namespace GraphCanvas
{
    // Deprecated name for the NumericDataInterface
    class DoubleDataInterface
        : public NumericDataInterface
    {
    public:
        AZ_DEPRECATED(DoubleDataInterface(), "DoubleDataInterface renamed to NumericDataInterface")
        {
        }

        virtual double GetDouble() const = 0;
        virtual void SetDouble(double value) = 0;

        double GetNumber() const override final
        {
            return GetDouble();
        }

        void SetNumber(double value) override final
        {
            SetDouble(value);
        }
    };
}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

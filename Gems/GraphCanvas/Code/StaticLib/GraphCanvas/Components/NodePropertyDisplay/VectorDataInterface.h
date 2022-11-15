/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "DataInterface.h"

#include <QPixmap>

namespace GraphCanvas
{
    // Thought: Potential to make this support arbitrary types of elements all in a single line.
    //          Might be useful eventually, for now sticking with pure numbers.
    class VectorDataInterface 
        : public DataInterface
    {
    public:
        using SubmitValueEvent = AZ::Event<>;

        // Returns the number of elements that this vector represents.
        virtual int GetElementCount() const = 0;
        
        // Sets the value for a particular index.
        virtual double GetValue(int index) const = 0;

        // Gets the value for a particular index.
        virtual void SetValue(int index, double value) = 0;
        
        // Gets the label for a particular index.
        virtual const char* GetLabel(int index) const = 0;

        // Gets the style for the object
        virtual AZStd::string GetStyle() const = 0;
        
        // Gets the style for a particular index
        virtual AZStd::string GetElementStyle(int index) const = 0;

        // Returns the precision for the given index.
        virtual int GetDecimalPlaces(int /*index*/) const { return 7; }

        // Returns the truncated display precision for the given index.
        virtual int GetDisplayDecimalPlaces(int /*index*/) const { return 4; }

        virtual void OnPressButton() { }
        virtual QPixmap GetIcon() const { return QPixmap(); }
        
        // Returns the minimum/maximum value for the given index.
        virtual double GetMinimum(int /*index*/) const { return -999999999; }
        virtual double GetMaximum(int /*index*/) const { return 999999999; }
        
        // Returns the suffix to append to the spin box.
        virtual const char* GetSuffix(int /*index*/) const { return ""; }
    };
}

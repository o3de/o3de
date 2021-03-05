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

namespace GraphCanvas
{
    // Thought: Potential to make this support arbitrary types of elements all in a single line.
    //          Might be useful eventually, for now sticking with pure numbers.
    class VectorDataInterface 
        : public DataInterface
    {
    public:
    
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
        virtual int GetDecimalPlaces([[maybe_unused]] int index) const { return 7; }

        // Returns the truncated display precision for the given index.
        virtual int GetDisplayDecimalPlaces([[maybe_unused]] int index) const { return 4; }
        
        // Returns the minimum/maximum value for the given index.
        virtual double GetMinimum([[maybe_unused]] int index) const { return -999999999; }
        virtual double GetMaximum([[maybe_unused]] int index) const { return 999999999; }
        
        // Returns the suffix to append to the spin box.
        virtual const char* GetSuffix([[maybe_unused]] int index) const { return ""; }        
    };
}
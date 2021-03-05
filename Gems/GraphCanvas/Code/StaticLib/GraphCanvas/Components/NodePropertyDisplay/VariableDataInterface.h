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
    class VariableReferenceDataInterface 
        : public DataInterface
    {
    public:
        // Returns the Uuid that represents the varaible assigned to this value.
        virtual AZ::Uuid GetVariableReference() const = 0;

        // Sets the entity reference that 
        virtual void AssignVariableReference(const AZ::Uuid& variableId) = 0;

        // Returns the type of variable that should be assigned to this value.
        virtual AZ::Uuid GetVariableDataType() const = 0;
    };
}
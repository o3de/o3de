/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

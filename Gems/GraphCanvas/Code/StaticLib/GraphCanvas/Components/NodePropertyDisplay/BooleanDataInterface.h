/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/DataInterface.h>

namespace GraphCanvas
{
    class BooleanDataInterface 
        : public DataInterface
    {
    public:
        virtual bool GetBool() const = 0;
        virtual void SetBool(bool value) = 0;
    };
}

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
    class GraphCanvasLabel;

    class ReadOnlyDataInterface 
        : public DataInterface
    {
    public:    
        virtual AZStd::string GetString() const = 0;        
    };
}

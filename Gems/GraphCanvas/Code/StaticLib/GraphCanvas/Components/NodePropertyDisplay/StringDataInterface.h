/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "DataInterface.h"

namespace GraphCanvas
{
    class StringDataInterface 
        : public DataInterface
    {
    public:    
        virtual AZStd::string GetString() const = 0;
        virtual void SetString(const AZStd::string& value) = 0;

        virtual bool ResizeToContents() const { return true; }
    };
}

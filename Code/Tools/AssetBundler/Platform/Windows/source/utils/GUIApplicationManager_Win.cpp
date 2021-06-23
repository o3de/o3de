/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/WindowDecorationWrapper.h>

namespace Platform
{
    AzQtComponents::WindowDecorationWrapper::Option GetWindowDecorationWrapperOption()
    {
        return AzQtComponents::WindowDecorationWrapper::OptionAutoAttach;
    }
}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/Components/StyleManager.h>

namespace AzQtComponents
{
    // Here for backwards compatibility with the old name of this class
    class O3DEStylesheet : public StyleManager
    {
    public:
        O3DEStylesheet(QObject* parent) : StyleManager(parent) {}
    };

} // namespace AzQtComponents


/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>

namespace AZ
{
    namespace Render
    {
        namespace PostFx
        {
            using LayerCategoriesMap = AZStd::map<AZStd::string, int>;
            static const char* DefaultLayerCategory = "Default";
            static const int DefaultLayerCategoryValue = std::numeric_limits<int>::max();
        }
    }
}

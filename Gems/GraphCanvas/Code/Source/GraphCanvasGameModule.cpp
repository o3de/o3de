/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvasModule.h>

namespace GraphCanvas
{
    GraphCanvasModule::GraphCanvasModule()
    {
    }

    AZ::ComponentTypeList GraphCanvasModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{};
    }
}

AZ_DECLARE_MODULE_CLASS(GraphCanvas_875b6fcbdeea44deaae7984ad9bb6cdc, GraphCanvas::GraphCanvasModule)

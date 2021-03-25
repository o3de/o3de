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

#include <precompiled.h>
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

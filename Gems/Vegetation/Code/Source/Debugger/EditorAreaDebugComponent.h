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

#include <AzCore/Component/TickBus.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>
#include <Debugger/AreaDebugComponent.h>

namespace Vegetation
{
    class EditorAreaDebugComponent
        : public EditorVegetationComponentBase<AreaDebugComponent, AreaDebugConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<AreaDebugComponent, AreaDebugConfig>;
        AZ_EDITOR_COMPONENT(EditorAreaDebugComponent, "{6B4591BA-6B8F-43D8-A311-22E83C7E8CB4}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Layer Debugger";
        static constexpr const char* const s_componentDescription = "Enables debug visualizations for vegetation layers";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-layer-debug";
    };
}
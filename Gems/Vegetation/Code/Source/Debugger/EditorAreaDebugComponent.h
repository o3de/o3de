/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}

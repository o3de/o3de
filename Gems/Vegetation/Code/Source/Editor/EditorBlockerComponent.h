/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Components/BlockerComponent.h>
#include <Vegetation/Editor/EditorAreaComponentBase.h>

namespace Vegetation
{
    class EditorBlockerComponent
        : public EditorAreaComponentBase<BlockerComponent, BlockerConfig>
    {
    public:
        using DerivedClassType = EditorBlockerComponent;
        using BaseClassType = EditorAreaComponentBase<BlockerComponent, BlockerConfig>;
        AZ_EDITOR_COMPONENT(EditorBlockerComponent, EditorBlockerComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Layer Blocker";
        static constexpr const char* const s_componentDescription = "Defines an area in which dynamic vegetation cannot be placed";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}

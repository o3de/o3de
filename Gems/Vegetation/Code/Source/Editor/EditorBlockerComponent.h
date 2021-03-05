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
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-layer-blocker";
    };
}
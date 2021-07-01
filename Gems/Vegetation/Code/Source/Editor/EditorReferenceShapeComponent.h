/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/ReferenceShapeComponent.h>
#include <Vegetation/Editor/EditorVegetationComponentBase.h>

namespace Vegetation
{
    class EditorReferenceShapeComponent
        : public EditorVegetationComponentBase<ReferenceShapeComponent, ReferenceShapeConfig>
    {
    public:
        using BaseClassType = EditorVegetationComponentBase<ReferenceShapeComponent, ReferenceShapeConfig>;
        AZ_EDITOR_COMPONENT(EditorReferenceShapeComponent, EditorReferenceShapeComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Vegetation";
        static constexpr const char* const s_componentName = "Vegetation Reference Shape";
        static constexpr const char* const s_componentDescription = "Enables the entity to reference and reuse shape entities";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Vegetation.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Vegetation.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
}

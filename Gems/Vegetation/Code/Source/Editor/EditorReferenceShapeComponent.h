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
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-reference-shape";
    };
}

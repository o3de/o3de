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

#include <Grid/GridComponent.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentBus.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        //! In-editor grid component
        class EditorGridComponent final
            : public EditorRenderComponentAdapter<GridComponentController, GridComponent, GridComponentConfig>
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<GridComponentController, GridComponent, GridComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorGridComponent, EditorGridComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorGridComponent() = default;
            EditorGridComponent(const GridComponentConfig& config);
        };
    } // namespace Render
} // namespace AZ

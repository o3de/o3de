/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

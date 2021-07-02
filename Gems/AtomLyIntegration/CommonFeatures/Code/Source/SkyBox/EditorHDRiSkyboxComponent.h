/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <SkyBox/HDRiSkyboxComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorHDRiSkyboxComponent final
            : public EditorRenderComponentAdapter<HDRiSkyboxComponentController, HDRiSkyboxComponent, HDRiSkyboxComponentConfig>
        {
        public:

            using BaseClass = EditorRenderComponentAdapter<HDRiSkyboxComponentController, HDRiSkyboxComponent, HDRiSkyboxComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorHDRiSkyboxComponent, EditorHDRiSkyboxComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorHDRiSkyboxComponent() = default;
            EditorHDRiSkyboxComponent(const HDRiSkyboxComponentConfig& config);
        };

    } // namespace Render
} // namespace AZ

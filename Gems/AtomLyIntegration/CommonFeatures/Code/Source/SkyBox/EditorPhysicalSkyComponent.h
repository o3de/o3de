/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <SkyBox/PhysicalSkyComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorPhysicalSkyComponent final
            : public EditorRenderComponentAdapter<PhysicalSkyComponentController, PhysicalSkyComponent, PhysicalSkyComponentConfig>
        {
        public:

            using BaseClass = EditorRenderComponentAdapter<PhysicalSkyComponentController, PhysicalSkyComponent, PhysicalSkyComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorPhysicalSkyComponent, EditorPhysicalSkyComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorPhysicalSkyComponent() = default;
            EditorPhysicalSkyComponent(const PhysicalSkyComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ

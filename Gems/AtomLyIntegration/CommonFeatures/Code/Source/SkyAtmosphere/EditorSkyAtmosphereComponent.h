/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <SkyAtmosphere/SkyAtmosphereComponent.h>

namespace AZ::Render
{
    class EditorSkyAtmosphereComponent final
        : public EditorRenderComponentAdapter<SkyAtmosphereComponentController, SkyAtmosphereComponent, SkyAtmosphereComponentConfig>
    {
    public:

        using BaseClass = EditorRenderComponentAdapter<SkyAtmosphereComponentController, SkyAtmosphereComponent, SkyAtmosphereComponentConfig>;
        AZ_EDITOR_COMPONENT(AZ::Render::EditorSkyAtmosphereComponent, "{9DE5B9CD-B2C8-4AB5-8D7D-21B43AAD56C3}", BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        EditorSkyAtmosphereComponent() = default;
        EditorSkyAtmosphereComponent(const SkyAtmosphereComponentConfig& config);

    private:
        //! EditorRenderComponentAdapter
        //! we override OnEntityVisibilityChanged to avoid deactivating and activating unnecessarily
        AZ::u32 OnConfigurationChanged() override;
        void OnEntityVisibilityChanged(bool visibility) override;
    };
} // namespace AZ::Render

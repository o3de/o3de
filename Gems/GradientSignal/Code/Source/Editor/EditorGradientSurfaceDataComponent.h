/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/GradientSurfaceDataComponent.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace GradientSignal
{
    class EditorGradientSurfaceDataComponent
        : public LmbrCentral::EditorWrappedComponentBase<GradientSurfaceDataComponent, GradientSurfaceDataConfig>
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<GradientSurfaceDataComponent, GradientSurfaceDataConfig>;
        AZ_EDITOR_COMPONENT(EditorGradientSurfaceDataComponent, "{4219B171-EF39-440E-B117-BA7FD914F93A}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        static constexpr const char* const s_categoryName = "Surface Data";
        static constexpr const char* const s_componentName = "Gradient Surface Tag Emitter";
        static constexpr const char* const s_componentDescription = "Enables a gradient to emit surface tags";
        static constexpr const char* const s_icon = "Editor/Icons/Components/SurfaceData.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/SurfaceData.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";
        
    private:
        AZ::u32 ConfigurationChanged() override;

        AZ::EntityId GetGradientEntityId() const;
        AZStd::function<float(float, const GradientSampleParams&)> GetFilterFunc();
        AZ::EntityId m_gradientEntityId;
    };
}

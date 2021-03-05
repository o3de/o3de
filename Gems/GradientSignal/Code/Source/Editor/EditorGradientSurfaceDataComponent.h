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
        static constexpr const char* const s_helpUrl = "https://docs.aws.amazon.com/console/lumberyard/surfacedata/gradient-surface-tag-emitter";
        
    private:
        AZ::u32 ConfigurationChanged() override;

        AZ::EntityId GetGradientEntityId() const;
        AZStd::function<float(float, const GradientSampleParams&)> GetFilterFunc();
        AZ::EntityId m_gradientEntityId;
    };
}

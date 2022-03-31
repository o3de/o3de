/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <Stars/StarsComponent.h>

namespace AZ::Render
{
    class EditorStarsComponent final
        : public EditorRenderComponentAdapter<StarsComponentController, StarsComponent, StarsComponentConfig>
    {
    public:
        using BaseClass = EditorRenderComponentAdapter<StarsComponentController, StarsComponent, StarsComponentConfig>;
        AZ_EDITOR_COMPONENT(AZ::Render::EditorStarsComponent, EditorStarsComponentTypeId, BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        EditorStarsComponent() = default;
        EditorStarsComponent(const StarsComponentConfig& config);

    private:
        //! EditorRenderComponentAdapter
        AZ::u32 OnConfigurationChanged() override;
        void OnEntityVisibilityChanged(bool visibility) override;

        AZ::Data::AssetId m_prevAssetId;
    };
} // namespace AZ::Render

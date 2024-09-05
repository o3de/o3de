/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainConstants.h>
#include <PostProcess/FilmGrain/FilmGrainComponentController.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/FilmGrain/FilmGrainComponentConfig.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        namespace FilmGrain
        {
            static constexpr const char* const FilmGrainComponentTypeId = "{E2F5CF7E-3D25-41E4-B3BF-C8669494F7B4}";
        }

        class FilmGrainComponent final
            : public AzFramework::Components::ComponentAdapter<FilmGrainComponentController, FilmGrainComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<FilmGrainComponentController, FilmGrainComponentConfig>;
            AZ_COMPONENT(AZ::Render::FilmGrainComponent, FilmGrain::FilmGrainComponentTypeId, BaseClass);

            FilmGrainComponent() = default;
            FilmGrainComponent(const FilmGrainComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ

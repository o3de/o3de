/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Material/MaterialComponentController.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>

#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        //! Can be paired with renderable components (MeshComponent for example)
        //! to provide material overrides on a per-entity basis.
        class MaterialComponent final
            : public AzFramework::Components::ComponentAdapter<MaterialComponentController, MaterialComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<MaterialComponentController, MaterialComponentConfig>;
            AZ_COMPONENT(MaterialComponent, MaterialComponentTypeId, BaseClass);

            MaterialComponent() = default;
            MaterialComponent(const MaterialComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ

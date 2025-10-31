/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <SpecularReflections/SpecularReflectionsComponentConfig.h>
#include <SpecularReflections/SpecularReflectionsComponentConstants.h>
#include <SpecularReflections/SpecularReflectionsComponentController.h>

namespace AZ
{
    namespace Render
    {
        class SpecularReflectionsComponent final
            : public AzFramework::Components::ComponentAdapter<SpecularReflectionsComponentController, SpecularReflectionsComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<SpecularReflectionsComponentController, SpecularReflectionsComponentConfig>;
            AZ_COMPONENT(AZ::Render::SpecularReflectionsComponent, SpecularReflectionsComponentTypeId , BaseClass);

            SpecularReflectionsComponent() = default;
            SpecularReflectionsComponent(const SpecularReflectionsComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ

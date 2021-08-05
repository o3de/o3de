/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DiffuseGlobalIllumination/DiffuseProbeGridComponentController.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGridComponent final
            : public AzFramework::Components::ComponentAdapter<DiffuseProbeGridComponentController, DiffuseProbeGridComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DiffuseProbeGridComponentController, DiffuseProbeGridComponentConfig>;
            AZ_COMPONENT(AZ::Render::DiffuseProbeGridComponent, DiffuseProbeGridComponentTypeId, BaseClass);

            DiffuseProbeGridComponent() = default;
            DiffuseProbeGridComponent(const DiffuseProbeGridComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ

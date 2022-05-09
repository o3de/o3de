/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CubeMapCapture/CubeMapCaptureComponentController.h>
#include <CubeMapCapture/CubeMapCaptureComponentConstants.h>
#include <AzFramework/Components/ComponentAdapter.h>

namespace AZ
{
    namespace Render
    {
        class CubeMapCaptureComponent final
            : public AzFramework::Components::ComponentAdapter<CubeMapCaptureComponentController, CubeMapCaptureComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<CubeMapCaptureComponentController, CubeMapCaptureComponentConfig>;
            AZ_COMPONENT(AZ::Render::CubeMapCaptureComponent, CubeMapCaptureComponentTypeId, BaseClass);

            CubeMapCaptureComponent() = default;
            CubeMapCaptureComponent(const CubeMapCaptureComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };

    } // namespace Render
} // namespace AZ

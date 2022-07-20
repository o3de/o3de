/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CubeMapCapture/CubeMapCaptureComponent.h>
#include <CubeMapCapture/CubeMapCaptureComponentConstants.h>
#include <CubeMapCapture/EditorCubeMapRenderer.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/CubeMapCapture/EditorCubeMapCaptureBus.h>

namespace AZ
{
    namespace Render
    {        
        class EditorCubeMapCaptureComponent final
            : public EditorRenderComponentAdapter<CubeMapCaptureComponentController, CubeMapCaptureComponent, CubeMapCaptureComponentConfig>
            , public EditorCubeMapCaptureBus::Handler
            , private EditorCubeMapRenderer
        {
        public:
            using BaseClass = EditorRenderComponentAdapter<CubeMapCaptureComponentController, CubeMapCaptureComponent, CubeMapCaptureComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorCubeMapCaptureComponent, EditorCubeMapCaptureComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorCubeMapCaptureComponent() = default;
            EditorCubeMapCaptureComponent(const CubeMapCaptureComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;

        private:
            // initiate the cubemap capture, returns the refresh value for the ChangeNotify UIElement attribute 
            AZ::u32 CaptureCubeMap() override;
        };

    } // namespace Render
} // namespace AZ

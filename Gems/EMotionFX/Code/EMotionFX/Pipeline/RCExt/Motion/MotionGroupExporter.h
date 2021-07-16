/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        struct MotionGroupExportContext;

        class MotionGroupExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(MotionGroupExporter, "{46AE1F54-6C71-405B-B63F-7BDCEAE8EB9B}", AZ::SceneAPI::SceneCore::ExportingComponent);

            explicit MotionGroupExporter();
            ~MotionGroupExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(MotionGroupExportContext& context) const;

        protected:
            static const AZStd::string  s_fileExtension;
        };
    } // namespace Pipeline
} // namespace EMotionFX

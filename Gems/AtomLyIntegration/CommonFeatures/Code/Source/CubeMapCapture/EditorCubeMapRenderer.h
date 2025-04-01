/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/SystemFile.h>
#include <Atom/Utils/DdsFile.h>
#include <Atom/Feature/CubeMapCapture/CubeMapCaptureFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        enum class CubeMapCaptureType : uint32_t
        {
            Specular,
            Diffuse
        };

        enum class CubeMapSpecularQualityLevel : uint32_t
        {
            VeryLow,    // 64
            Low,        // 128
            Medium,     // 256
            High,       // 512
            VeryHigh,   // 1024

            Count
        };

        static const char* CubeMapSpecularFileSuffixes[] =
        {
            "_iblspecularcm64.dds",
            "_iblspecularcm128.dds",
            "_iblspecularcm256.dds",
            "_iblspecularcm512.dds",
            "_iblspecularcm1024.dds"
        };

        static_assert(AZ_ARRAY_SIZE(CubeMapSpecularFileSuffixes) == aznumeric_cast<uint32_t>(CubeMapSpecularQualityLevel::Count),
            "CubeMapSpecularFileSuffixes must have the same number of entries as CubeMapSpecularQualityLevel");

        static constexpr const char* CubeMapDiffuseFileSuffix = "_ibldiffusecm.dds";

        // Mixin class that provides cubemap capture capability for editor components
        class EditorCubeMapRenderer
        {
        protected:
            EditorCubeMapRenderer() = default;

            // initiate the cubemap render and update the relative path if necessary
            AZ::u32 RenderCubeMap(
                AZStd::function<void(RenderCubeMapCallback, AZStd::string&)> renderCubeMapFn,
                const AZStd::string dialogText,
                const AZ::Entity* entity,
                const AZStd::string& folderName,
                AZStd::string& relativePath,
                CubeMapCaptureType captureType,
                CubeMapSpecularQualityLevel specularQualityLevel = CubeMapSpecularQualityLevel::Medium);

        private:
            // save the cubemap data to the output file
            void WriteOutputFile(AZStd::string filePath, uint8_t* const* cubeMapTextureData, RHI::Format cubeMapTextureFormat);

            // flag indicating if a cubemap render is currently in progress
            AZStd::atomic_bool m_renderInProgress = false;
        };
    } // namespace Render
} // namespace AZ

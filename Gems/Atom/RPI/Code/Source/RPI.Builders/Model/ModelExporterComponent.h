/*
 * All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution(the "License").All use of this software is governed by the License,
 *or, if provided, by the license below or the license accompanying this file.Do not
 * remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Serialization/Utils.h>

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            class ExportEventContext;
        }
    }

    namespace RPI
    {
        /**
         * This is the central component that drive the process of exporting a scene to Model 
         * and Material assets. It delegates asset-build duties to other components like
         * ModelAssetBuilderComponent and MaterialAssetBuilderComponent via export events.
         */
        class ModelExporterComponent : public SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(
                ModelExporterComponent,
                "{AE42AB62-A4D6-4147-88A0-692549EE5427}",
                SceneAPI::SceneCore::ExportingComponent);
            static void Reflect(ReflectContext* context);

            ModelExporterComponent();
            ~ModelExporterComponent() override = default;

            SceneAPI::Events::ProcessingResult ExportModel(SceneAPI::Events::ExportEventContext& context) const;

        private:
            struct AssetExportContext
            {
                AssetExportContext() = default;
                AssetExportContext(
                    AZStd::string_view relativeFileName,
                    AZStd::string_view extension,
                    Uuid sourceUuid,
                    DataStream::StreamType dataStreamType);

                AZStd::string_view m_relativeFileName;
                AZStd::string_view m_extension;
                const Uuid m_sourceUuid;
                const DataStream::StreamType m_dataStreamType = DataStream::ST_BINARY;
            };

            template<class T>
            bool ExportAsset(
                const Data::Asset<T>& asset,
                const AssetExportContext& assetContext,
                SceneAPI::Events::ExportEventContext& eventContext,
                const char* assetTypeDebugName) const;
        };
    } // namespace RPI
} // namespace AZ

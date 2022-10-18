/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Shader/ShaderUtils.h>
//#include <Atom/RPI.Edit/Common/AssetUtils.h>
//#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
//#include <Atom/RPI.Reflect/Image/ImageAsset.h>
//#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
//#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
//#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
//#include <Atom/RPI.Edit/Material/MaterialPipelineSourceData.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonImporter.h>
//#include <AzCore/Settings/SettingsRegistry.h>
//
//#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RPI
    {
        namespace ShaderUtils
        {
            Outcome<RPI::ShaderSourceData, AZStd::string> LoadShaderDataJson(const AZStd::string& fullPathToJsonFile)
            {
                RPI::ShaderSourceData shaderSourceData;

                AZ::Outcome<rapidjson::Document, AZStd::string> loadOutcome =
                    JsonSerializationUtils::ReadJsonFile(fullPathToJsonFile, AZ::RPI::JsonUtils::DefaultMaxFileSize);

                if (!loadOutcome.IsSuccess())
                {
                    return Failure(loadOutcome.GetError());
                }

                rapidjson::Document document = loadOutcome.TakeValue();

                JsonDeserializerSettings settings;
                RPI::JsonReportingHelper reportingHelper;
                reportingHelper.Attach(settings);

                JsonSerialization::Load(shaderSourceData, document, settings);

                if (reportingHelper.WarningsReported() || reportingHelper.ErrorsReported())
                {
                    return AZ::Failure(reportingHelper.GetErrorMessage());
                }

                return AZ::Success(shaderSourceData);
            }

        }
    }
}

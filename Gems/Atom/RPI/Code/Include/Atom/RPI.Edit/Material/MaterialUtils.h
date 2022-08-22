/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    class JsonDeserializerContext;

    namespace JsonSerializationResult
    {
        union ResultCode;
    }

    namespace RPI
    {
        class MaterialSourceData;
        class MaterialTypeSourceData;

        namespace MaterialUtils
        {
            using ImportedJsonFiles = AZStd::unordered_set<AZStd::string>;

            enum class GetImageAssetResult
            {
                Empty,             //! No image was actually requested, the path was empty
                Found,             //! The requested asset was found
                Missing            //! The requested asset was not found, and a placeholder asset was used instead
            };

            //! Finds an ImageAsset referenced by a material file (or a placeholder)
            //! @param imageAsset the resulting ImageAsset
            //! @param materialSourceFilePath the full path to a material source file that is referenfing an image file
            //! @param imageFilePath the path to an image source file, which could be relative to the asset root or relative to the material file
            GetImageAssetResult GetImageAssetReference(Data::Asset<ImageAsset>& imageAsset, AZStd::string_view materialSourceFilePath, const AZStd::string imageFilePath);

            //! Resolve an enum to a uint32_t given its name and definition array (in MaterialPropertyDescriptor).
            //! @param propertyDescriptor it contains the definition of all enum names in an array.
            //! @param enumName the name of an enum to be converted into a uint32_t.
            //! @param outResolvedValue where the correct resolved value will be set into as a uint32_t.
            //! @return if resolving is successful. An error will be reported if it fails.
            bool ResolveMaterialPropertyEnumValue(const MaterialPropertyDescriptor* propertyDescriptor, const AZ::Name& enumName, MaterialPropertyValue& outResolvedValue);

            //! Load a material type from a json file or document.
            //! Otherwise, it will use the passed in document first if not null, or load the json document from the path.
            //! @param filePath path to the JSON file to load, unless the @document is already provided. In either case, this path will be used to resolve any relative file references.
            //! @param document an optional already loaded json document.
            //! @param importedFiles receives the list of files that were imported by the JSON serializer
            AZ::Outcome<MaterialTypeSourceData> LoadMaterialTypeSourceData(const AZStd::string& filePath, rapidjson::Document* document = nullptr, ImportedJsonFiles* importedFiles = nullptr);

            //! Load a material from a json file.
            AZ::Outcome<MaterialSourceData> LoadMaterialSourceData(const AZStd::string& filePath, const rapidjson::Value* document = nullptr, bool warningsAsErrors = false);

            //! Utility function for custom JSON serializers to report results as "Skipped" when encountering keys that aren't recognized
            //! as part of the custom format.
            //! @param acceptedFieldNames an array of names that are recognized by the custom format
            //! @param acceptedFieldNameCount the number of elements in @acceptedFieldNames
            //! @param object the JSON object being loaded
            //! @param context the common JsonDeserializerContext that is central to the serialization process
            //! @param result the ResultCode that well be updated with the Outcomes "Skipped" if an unrecognized field is encountered
            void CheckForUnrecognizedJsonFields(
                const AZStd::string_view* acceptedFieldNames, uint32_t acceptedFieldNameCount,
                const rapidjson::Value& object, JsonDeserializerContext& context, JsonSerializationResult::ResultCode& result);

            //! Materials assets can either be finalized during asset-processing time or when materials are loaded at runtime.
            //! Finalizing during asset processing reduces load times and obfuscates the material data.
            //! Waiting to finalize at load time reduces dependencies on the material type data, resulting in fewer asset rebuilds and less time spent processing assets.
            //! Removing the dependency on the material type data will require special handling of material type asset dependencies when loading and reloading materials.
            bool BuildersShouldFinalizeMaterialAssets();
        }
    }
}

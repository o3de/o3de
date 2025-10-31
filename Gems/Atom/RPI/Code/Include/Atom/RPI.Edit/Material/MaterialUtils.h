/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Configuration.h>
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
        struct MaterialPipelineSourceData;

        namespace MaterialUtils
        {
            using ImportedJsonFiles = AZStd::unordered_set<AZ::IO::Path>;

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
            ATOM_RPI_EDIT_API GetImageAssetResult GetImageAssetReference(Data::Asset<ImageAsset>& imageAsset, AZStd::string_view materialSourceFilePath, const AZStd::string imageFilePath);

            //! Resolve an enum to a uint32_t given its name and definition array (in MaterialPropertyDescriptor).
            //! @param propertyDescriptor it contains the definition of all enum names in an array.
            //! @param enumName the name of an enum to be converted into a uint32_t.
            //! @param outResolvedValue where the correct resolved value will be set into as a uint32_t.
            //! @return if resolving is successful. An error will be reported if it fails.
            ATOM_RPI_EDIT_API bool ResolveMaterialPropertyEnumValue(const MaterialPropertyDescriptor* propertyDescriptor, const AZ::Name& enumName, MaterialPropertyValue& outResolvedValue);

            //! Load a material pipeline file from a json file or document.
            //! It will use the passed in document if not null, or otherwise load the json document from the path.
            //! @param filePath path to the JSON file to load, unless the @document is already provided. In either case, this path will be used to resolve any relative file references.
            //! @param document an optional already loaded json document.
            //! @param importedFiles receives the list of files that were imported by the JSON serializer
            ATOM_RPI_EDIT_API AZ::Outcome<MaterialPipelineSourceData> LoadMaterialPipelineSourceData(const AZStd::string& filePath, rapidjson::Document* document = nullptr, ImportedJsonFiles* importedFiles = nullptr);

            //! Load a material type from a json file or document.
            //! It will use the passed in document if not null, or otherwise load the json document from the path.
            //! @param filePath path to the JSON file to load, unless the @document is already provided. In either case, this path will be used to resolve any relative file references.
            //! @param document an optional already loaded json document.
            //! @param importedFiles receives the list of files that were imported by the JSON serializer
            ATOM_RPI_EDIT_API AZ::Outcome<MaterialTypeSourceData> LoadMaterialTypeSourceData(const AZStd::string& filePath, rapidjson::Document* document = nullptr, ImportedJsonFiles* importedFiles = nullptr);

            //! Load a material from a json file.
            ATOM_RPI_EDIT_API AZ::Outcome<MaterialSourceData> LoadMaterialSourceData(const AZStd::string& filePath, const rapidjson::Value* document = nullptr, bool warningsAsErrors = false);

            //! Utility function for custom JSON serializers to report results as "Skipped" when encountering keys that aren't recognized
            //! as part of the custom format.
            //! @param acceptedFieldNames an array of names that are recognized by the custom format
            //! @param acceptedFieldNameCount the number of elements in @acceptedFieldNames
            //! @param object the JSON object being loaded
            //! @param context the common JsonDeserializerContext that is central to the serialization process
            //! @param result the ResultCode that well be updated with the Outcomes "Skipped" if an unrecognized field is encountered
            ATOM_RPI_EDIT_API void CheckForUnrecognizedJsonFields(
                const AZStd::string_view* acceptedFieldNames, uint32_t acceptedFieldNameCount,
                const rapidjson::Value& object, JsonDeserializerContext& context, JsonSerializationResult::ResultCode& result);

            //! Inspects the content of the MaterialPropertyValue to see if it is a string that appears to be an image file path.
            ATOM_RPI_EDIT_API bool LooksLikeImageFileReference(const MaterialPropertyValue& value);

            //! Returns whether the name is a valid C-style identifier
            ATOM_RPI_EDIT_API bool IsValidName(AZStd::string_view name);
            ATOM_RPI_EDIT_API bool IsValidName(const AZ::Name& name);

            //! Returns whether the name is a valid C-style identifier, and reports errors if it is not.
            ATOM_RPI_EDIT_API bool CheckIsValidPropertyName(AZStd::string_view name);
            ATOM_RPI_EDIT_API bool CheckIsValidGroupName(AZStd::string_view name);

            //! Convert an intermediate material type path back to the original source material type path.
            //! @param materialTypeSourcePath Path to material type source in intermediate asset folder.
            //! @return Absolute path to original material type or original input path.
            ATOM_RPI_EDIT_API AZStd::string PredictOriginalMaterialTypeSourcePath(const AZStd::string& materialTypeSourcePath);

            //! Returns the file path that would be used for an intermediate .materialtype, if there were one, for the given .materialtype path.
            //! @param originalMaterialTypeSourcePath the path to the .materialtype
            //! @param referencingFilePath The @originalMaterialTypeSourcePath can be relative to this path in addition to the asset root.
            ATOM_RPI_EDIT_API AZStd::string PredictIntermediateMaterialTypeSourcePath(const AZStd::string& originalMaterialTypeSourcePath);
            ATOM_RPI_EDIT_API AZStd::string PredictIntermediateMaterialTypeSourcePath(const AZStd::string& referencingFilePath, const AZStd::string& originalMaterialTypeSourcePath);

            //! Checks to see if there is an intermediate .materialtype for the given .materialtype path, and returns its path.
            ATOM_RPI_EDIT_API AZStd::string GetIntermediateMaterialTypeSourcePath(const AZStd::string& forOriginalMaterialTypeSourcePath);

            //! Returns the path of the final .materialtype path, which may be the same as the original path, or could be a different path if there is
            //! an intermediate .materialtype file associated with this one.
            //! @param originalMaterialTypeSourcePath the path to the .materialtype
            //! @param referencingFilePath The @originalMaterialTypeSourcePath can be relative to this path in addition to the asset root.
            ATOM_RPI_EDIT_API AZStd::string GetFinalMaterialTypeSourcePath(const AZStd::string& referencingFilePath, const AZStd::string& originalMaterialTypeSourcePath);

            //! Returns AssetId of the MaterialTypeAsset from the given .materialtype source file path. This will account for the possibility that there
            //! is an intermediate .materialtype file associated with the given .materialtype.
            //! @param originalMaterialTypeSourcePath the path to the .materialtype
            //! @param referencingFilePath The @originalMaterialTypeSourcePath can be relative to this path in addition to the asset root.
            ATOM_RPI_EDIT_API Outcome<Data::AssetId> GetFinalMaterialTypeAssetId(const AZStd::string& referencingFilePath, const AZStd::string& originalMaterialTypeSourcePath);
        }
    }
}

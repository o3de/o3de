/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    namespace RPI
    {
        class MaterialTypeSourceData;

        namespace MaterialUtils
        {
            Outcome<Data::Asset<ImageAsset>> GetImageAssetReference(AZStd::string_view materialSourceFilePath, const AZStd::string imageFilePath);

            //! Resolve an enum to a uint32_t given its name and definition array (in MaterialPropertyDescriptor).
            //! @param propertyDescriptor it contains the definition of all enum names in an array.
            //! @param enumName the name of an enum to be converted into a uint32_t.
            //! @param outResolvedValue where the correct resolved value will be set into as a uint32_t.
            //! @return if resolving is successful. An error will be reported if it fails.
            bool ResolveMaterialPropertyEnumValue(const MaterialPropertyDescriptor* propertyDescriptor, const AZ::Name& enumName, MaterialPropertyValue& outResolvedValue);

            //! Load material type from a json file. If the file path is relative, the loaded json document must be provided.
            //! Otherwise, it will use the passed in document first if not null, or load the json document from the path.
            //! @param filePath a relative path if document is provided, an absolute path if document is not provided.
            //! @param document the loaded json document.
            AZ::Outcome<MaterialTypeSourceData> LoadMaterialTypeSourceData(const AZStd::string& filePath, const rapidjson::Value* document = nullptr);
        }
    }
}

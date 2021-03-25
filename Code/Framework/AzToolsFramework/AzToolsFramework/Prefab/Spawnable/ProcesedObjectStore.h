/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    //! Storage for objects created through the Prefab processing pipeline.
    //! These typically store the created object for immediate use in the editor plus additional information
    //! to allow the Prefab Builder to convert the object into a serialized form and register it with the
    //! Asset Database.
    class ProcessedObjectStore
    {
    public:
        using SerializerFunction = AZStd::function<bool(AZStd::vector<uint8_t>&, const ProcessedObjectStore&)>;

        //! Constructs a new instance.
        //! @param uniqueId A name for the object that's unique within the scope of the Prefab. This name will be used to generate a sub id for the product
        //!     which requires that the name is stable between runs.
        //! @param object The object that generated during processing of a Prefab.
        //! @param objectSerializer The callback used to convert the provided object into a binary stream.
        //! @param assetType The asset type of the asset.
        //! @param storagePath The relative path where the asset will be stored if/when committed to disk.
        ProcessedObjectStore(AZStd::string uniqueId, AZStd::any object, SerializerFunction objectSerializer, AZ::Data::AssetType assetType);

        bool Serialize(AZStd::vector<uint8_t>& output) const;
        uint32_t BuildSubId() const;

        const AZStd::any& GetObject() const;
        AZStd::any ReleaseObject();

        const AZ::Data::AssetType& GetAssetType() const;
        const AZStd::string& GetId() const;

    private:
        AZStd::any m_object;
        SerializerFunction m_objectSerializer;
        AZ::Data::AssetType m_assetType;
        AZStd::string m_uniqueId;
    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils

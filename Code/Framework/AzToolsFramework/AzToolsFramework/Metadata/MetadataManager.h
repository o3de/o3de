/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/IO/FileReader.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/JSON/pointer.h>

namespace AzToolsFramework
{
    struct IMetadataRequests
    {
        AZ_RTTI(IMetadataRequests, "{88EB4C71-F2A5-48C6-A08B-0D6A8EB08A74}");

        virtual ~IMetadataRequests() = default;

        virtual bool Get(AZ::IO::PathView file, AZStd::string_view key, void* outValue, AZ::Uuid typeId) = 0;
        virtual bool Set(AZ::IO::PathView file, AZStd::string_view key, const void* inValue, AZ::Uuid typeId) = 0;
    };
#pragma optimize("", off)
    class MetadataManager :
        public AZ ::Component,
        public AZ::Interface<IMetadataRequests>::Registrar
    {
    public:
        AZ_COMPONENT(MetadataManager, "{CB738803-3B6C-4B62-9DC2-1980D340F288}", IMetadataRequests);

        static inline constexpr const char* MetadataFileExtension = ".metadata";

        static void Reflect(AZ::ReflectContext* )
        {
        }

    protected:
        void Activate() override{}
        void Deactivate() override{}

    public:
        bool Get(AZ::IO::PathView file, AZStd::string_view key, void* outValue, AZ::Uuid typeId) override
        {
            auto path = ToMetadataPath(file);

            auto result = AZ::JsonSerializationUtils::ReadJsonFile(path.Native());

            if(!result)
            {
                return false;
            }

            auto& document = result.GetValue();

            rapidjson_ly::Pointer pointer(key.data(), key.length());

            if(!pointer.IsValid())
            {
                return false;
            }

            const rapidjson_ly::Value* value = pointer.Get(document);

            if(!value)
            {
                return false;
            }

            auto resultCode = AZ::JsonSerialization::Load(outValue, typeId, *value);

            return resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted;
        }

        bool Set(AZ::IO::PathView file, AZStd::string_view key, const void* inValue, AZ::Uuid typeId) override
        {
            auto path = ToMetadataPath(file);

            rapidjson_ly::Pointer pointer(key.data(), key.length());

            if (!pointer.IsValid())
            {
                return false;
            }

            auto result = AZ::JsonSerializationUtils::ReadJsonFile(path.Native());

            rapidjson_ly::Document document;

            if (result)
            {
                document = result.TakeValue();
            }

            rapidjson_ly::Value serializedValue;
            auto resultCode = AZ::JsonSerialization::Store(serializedValue, document.GetAllocator(), inValue, nullptr, typeId);

            if(resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted)
            {
                rapidjson_ly::Value& store = pointer.Create(document, document.GetAllocator());
                store = AZStd::move(serializedValue);

                auto saveResult = AZ::JsonSerializationUtils::WriteJsonFile(document, path.Native());

                return saveResult.IsSuccess();
            }

            return false;
        }

    private:
        AZ::IO::Path ToMetadataPath(AZ::IO::PathView file)
        {
            AZ::IO::Path path = file;

            if(path.Extension() != MetadataFileExtension)
            {
                path.ReplaceExtension(AZ::IO::PathView(AZStd::string(path.Extension().Native()) + AZStd::string(MetadataFileExtension)));
            }

            return path;
        }

        //AZStd::optional<rapidjson_ly::Document> LoadFile(AZ::IO::PathView file) const
        //{
        //    AZ::IO::FileIOStream stream;

        //    if(!stream.Open(file.FixedMaxPathString().c_str(), AZ::IO::OpenMode::ModeRead))
        //    {
        //        return {};
        //    }

        //    Metadata metadata;
        //    bool result = AZ::Utils::LoadObjectFromStreamInPlace(stream, metadata);

        //    if(!result)
        //    {
        //        return {};
        //    }

        //    return metadata;
        //}
    };
#pragma optimize("", on)
} // namespace AzToolsFramework

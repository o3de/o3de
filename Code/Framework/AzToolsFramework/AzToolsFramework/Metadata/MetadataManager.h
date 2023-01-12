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
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    struct IMetadataRequests
    {
        AZ_RTTI(IMetadataRequests, "{88EB4C71-F2A5-48C6-A08B-0D6A8EB08A74}");

        virtual ~IMetadataRequests() = default;

        //! Gets a value from the metadata file associated with the file for the given key.
        //! @param file Absolute path to the file (or metadata file).
        //! @param key JSONPath formatted key for the metadata value to read.  Ex: /Settings/Platform/pc.
        //! @param typeId Type of the stored value and outValue.
        //! @return True if metadata file and key exists and was successfully read, false otherwise.
        virtual bool GetValue(AZ::IO::PathView file, AZStd::string_view key, void* outValue, AZ::Uuid typeId) = 0;

        //! Gets a value from the metadata file associated with the file for the given key.
        //! @param file Absolute path to the file (or metadata file).
        //! @param key JSONPath formatted key for the metadata value to read.  Ex: /Settings/Platform/pc.
        //! @return True if metadata file and key exists and was successfully read, false otherwise.
        template<typename T>
        bool GetValue(AZ::IO::PathView file, AZStd::string_view key, T& outValue)
        {
            return GetValue(file, key, &outValue, azrtti_typeid<T>());
        }

        //! Gets the raw JSON from the metadata file associated with the file for the given key.
        //! Prefer to use GetValue instead where possible.  This is best used for reading old versions.
        //! @param file Absolute path to the file (or metadata file).
        //! @param key JSONPath formatted key for the metadata value to read.  Ex: /Settings/Platform/pc.
        //! @return True if metadata file and key exists and was successfully read, false otherwise.
        virtual bool GetJson(AZ::IO::PathView file, AZStd::string_view key, rapidjson::Document& outValue) = 0;

        //! Gets the version for a stored key/value from the metadata file associated with the file.
        //! @param file Absolute path to the file (or metadata file).
        //! @param key JSONPath formatted key for the metadata value to read.  Ex: /Settings/Platform/pc.
        //! @return True if metadata file and key exists and was successfully read, false otherwise.
        virtual bool GetValueVersion(AZ::IO::PathView file, AZStd::string_view key, int& version) = 0;

        //! Sets a value in the metadata file associated with the file.
        //! @param file Absolute path to the file (or metadata file).
        //! @param key JSONPath formatted key for the metadata value to write.  Ex: /Settings/Platform/pc.
        //! @param typeId Type of the stored value and inValue.
        //! @return True if the value is successfully saved to disk, false otherwise.
        virtual bool SetValue(AZ::IO::PathView file, AZStd::string_view key, const void* inValue, AZ::Uuid typeId) = 0;

        //! Sets a value in the metadata file associated with the file.
        //! @param file Absolute path to the file (or metadata file).
        //! @param key JSONPath formatted key for the metadata value to write.  Ex: /Settings/Platform/pc.
        //! @return True if the value is successfully saved to disk, false otherwise.
        template<typename T>
        bool SetValue(AZ::IO::PathView file, AZStd::string_view key, const T& inValue)
        {
            return SetValue(file, key, &inValue, azrtti_typeid<T>());
        }
    };

    //! Component that handles reading/writing to metadata files.
    //! Metadata files are stored alongside source assets and can contain any generic data that needs to be associated with a file.
    class MetadataManager :
        public AZ::Component,
        public AZ::Interface<IMetadataRequests>::Registrar
    {
    public:
        AZ_COMPONENT(MetadataManager, "{CB738803-3B6C-4B62-9DC2-1980D340F288}", IMetadataRequests);

        static constexpr const char* MetadataFileExtension = ".meta";
        static constexpr const char* MetadataVersionKey = "/FileVersion";
        static constexpr const char* MetadataObjectVersionField = "__version";
        static constexpr int MetadataVersion = 1;

        static void Reflect(AZ::ReflectContext* context);
        static AZ::IO::Path ToMetadataPath(AZ::IO::Path path);

    protected:
        void Activate() override{}
        void Deactivate() override{}

    public:
        bool GetValue(AZ::IO::PathView file, AZStd::string_view key, void* outValue, AZ::Uuid typeId) override;
        bool GetJson(AZ::IO::PathView file, AZStd::string_view key, rapidjson::Document& outValue) override;
        bool GetValueVersion(AZ::IO::PathView file, AZStd::string_view key, int& version) override;
        bool SetValue(AZ::IO::PathView file, AZStd::string_view key, const void* inValue, AZ::Uuid typeId) override;

    private:
        const AZ::SerializeContext::ClassData* GetClassData(AZ::Uuid typeId);
    };
} // namespace AzToolsFramework

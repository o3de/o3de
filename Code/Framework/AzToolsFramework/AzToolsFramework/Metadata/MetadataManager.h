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

    class MetadataManager :
        public AZ ::Component,
        public AZ::Interface<IMetadataRequests>::Registrar
    {
    public:
        AZ_COMPONENT(MetadataManager, "{CB738803-3B6C-4B62-9DC2-1980D340F288}", IMetadataRequests);

        static inline constexpr const char* MetadataFileExtension = ".metadata";
        static inline constexpr const char* MetadataVersionKey = "/FileVersion";
        static inline constexpr int MetadataVersion = 1;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        void Activate() override{}
        void Deactivate() override{}

    public:
        bool Get(AZ::IO::PathView file, AZStd::string_view key, void* outValue, AZ::Uuid typeId) override;
        bool Set(AZ::IO::PathView file, AZStd::string_view key, const void* inValue, AZ::Uuid typeId) override;

    private:
        AZ::IO::Path ToMetadataPath(AZ::IO::PathView file);
    };
} // namespace AzToolsFramework

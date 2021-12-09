/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzCore/DOM/DomBackend.h>
#include <AzCore/IO/ByteContainerStream.h>

namespace AZ::Dom
{
    //! A DOM backend for serializing and deserializing JSON <=> UTF-8 text
    //! \param ParseFlags Controls how deserialized JSON is parsed.
    //! \param WriteFormat Controls how serialized JSON is formatted.
    template<
        Json::ParseFlags ParseFlags = Json::ParseFlags::ParseComments,
        Json::OutputFormatting WriteFormat = Json::OutputFormatting::PrettyPrintedJson>
    class JsonBackend final : public Backend
    {
    public:
        Visitor::Result ReadFromBuffer(const char* buffer, size_t size, AZ::Dom::Lifetime lifetime, Visitor& visitor) override
        {
            return Json::VisitSerializedJson<ParseFlags>({ buffer, size }, lifetime, visitor);
        }

        Visitor::Result ReadFromBufferInPlace(char* buffer, [[maybe_unused]] AZStd::optional<size_t> size, Visitor& visitor) override
        {
            return Json::VisitSerializedJsonInPlace<ParseFlags>(buffer, visitor);
        }

        Visitor::Result WriteToBuffer(AZStd::string& buffer, WriteCallback callback)
        {
            AZ::IO::ByteContainerStream<AZStd::string> stream{ &buffer };
            AZStd::unique_ptr<Visitor> visitor = Json::CreateJsonStreamWriter(stream, WriteFormat);
            return callback(*visitor);
        }
    };
} // namespace AZ::Dom

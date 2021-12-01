/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonBackend.h>

namespace AZ::Dom
{
    Visitor::Result JsonBackend::ReadFromStringInPlace(AZStd::string& buffer, Visitor& visitor)
    {
        return Json::VisitSerializedJsonInPlace(buffer, visitor);
    }

    Visitor::Result JsonBackend::ReadFromString(AZStd::string_view buffer, Lifetime lifetime, Visitor& visitor)
    {
        return Json::VisitSerializedJson(buffer, lifetime, visitor);
    }

    AZStd::unique_ptr<Visitor> JsonBackend::CreateStreamWriter(AZ::IO::GenericStream& stream)
    {
        return Json::GetJsonStreamWriter(stream, Json::OutputFormatting::PrettyPrintedJson);
    }
}

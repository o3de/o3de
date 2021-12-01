/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>
#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>

namespace AZ::Dom
{
    class JsonBackend final : public Backend
    {
    public:
        Visitor::Result ReadFromStringInPlace(AZStd::string& buffer, Visitor& visitor) override;
        Visitor::Result ReadFromString(AZStd::string_view buffer, Lifetime lifetime, Visitor& visitor) override;
        AZStd::unique_ptr<Visitor> CreateStreamWriter(AZ::IO::GenericStream& stream) override;
    };
} // namespace AZ::Dom

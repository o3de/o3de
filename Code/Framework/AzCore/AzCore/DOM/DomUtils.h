/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>

namespace AZ::Dom::Utils
{
    Visitor::Result ReadFromString(Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime, Visitor& visitor);
    Visitor::Result ReadFromStringInPlace(Backend& backend, AZStd::string& string, Visitor& visitor);
} // namespace AZ::Dom::Utils

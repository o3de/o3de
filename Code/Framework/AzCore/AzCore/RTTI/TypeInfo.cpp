/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/TypeInfo.h>

//! Add GetO3deTypeName and GetO3deTypeId definitions for commonly used O3DE types
namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(AZ::Uuid, "AZ::Uuid", "{E152C105-A133-4d03-BBF8-3D4B2FBA3E2A}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(PlatformID, "PlatformID", "{0635D08E-DDD2-48DE-A7AE-73CC563C57C3}");
}

namespace AZStd
{
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(AZStd::monostate, "AZStd::monostate", "{B1E9136B-D77A-4643-BE8E-2ABDA246AE0E}");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(AZStd::allocator, "allocator", "{E9F5A3BE-2B3D-4C62-9E6B-4E00A13AB452}");
}

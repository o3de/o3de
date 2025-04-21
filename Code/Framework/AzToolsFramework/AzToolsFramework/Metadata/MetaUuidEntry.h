/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    //! Structure used to store UUID information for an asset in the metadata file
    struct MetaUuidEntry
    {
        AZ_TYPE_INFO(MetaUuidEntry, "{FAD60D80-9B1D-421D-A4CA-DD2CA2EA80BB}");

        static void Reflect(AZ::ReflectContext* context);

        // The canonical UUID
        AZ::Uuid m_uuid;
        // A list of UUIDs that used to refer to this file
        AZStd::unordered_set<AZ::Uuid> m_legacyUuids;
        // The relative path of the file when it was originally created
        AZStd::string m_originalPath;
        // Creation time of the UUID entry
        AZ::u64 m_millisecondsSinceUnixEpoch{};
    };
} // namespace AzToolsFramework

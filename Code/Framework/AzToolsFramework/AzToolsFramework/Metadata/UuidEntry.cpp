/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Metadata/UuidEntry.h>

namespace AzToolsFramework
{
    void AzToolsFramework::UuidEntry::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<UuidEntry>()
                ->Version(0)
                ->Field("uuid", &UuidEntry::m_uuid)
                ->Field("legacyUuids", &UuidEntry::m_legacyUuids)
                ->Field("originalPath", &UuidEntry::m_originalPath)
                ->Field("creationUnixEpochMS", &UuidEntry::m_millisecondsSinceUnixEpoch);
        }
    }
} // namespace AzToolsFramework

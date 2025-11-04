/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Metadata/MetaUuidEntry.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    void AzToolsFramework::MetaUuidEntry::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MetaUuidEntry>()
                ->Version(0)
                ->Field("uuid", &MetaUuidEntry::m_uuid)
                ->Field("legacyUuids", &MetaUuidEntry::m_legacyUuids)
                ->Field("originalPath", &MetaUuidEntry::m_originalPath)
                ->Field("creationUnixEpochMS", &MetaUuidEntry::m_millisecondsSinceUnixEpoch);
        }
    }
} // namespace AzToolsFramework

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    //! Return whether component should appear in an entity's "Add Component" menu.
    //! \param entityType The type of entity (ex: "Game", "System")
    AZTF_API bool AppearsInAddComponentMenu(const AZ::SerializeContext::ClassData& classData, const AZ::Crc32& entityType);

    //! ComponentFilter for components that users can add to game entities.
    AZTF_API bool AppearsInGameComponentMenu(const AZ::SerializeContext::ClassData&);

    inline bool AppearsInLayerComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        return AppearsInAddComponentMenu(classData, AZ_CRC_CE("Layer"));
    }

    inline bool AppearsInLevelComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        return AppearsInAddComponentMenu(classData, AZ_CRC_CE("Level"));
    }

    inline bool AppearsInAnyComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        return (AppearsInGameComponentMenu(classData) || AppearsInLayerComponentMenu(classData) || AppearsInLevelComponentMenu(classData));
    }
} // AzToolsFramework

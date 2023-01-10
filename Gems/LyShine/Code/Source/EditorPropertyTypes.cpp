/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorPropertyTypes.h"

#include <LyShine/Bus/UiIndexableImageBus.h>

#include <AzCore/EBus/EBus.h>

LyShine::AZu32ComboBoxVec LyShine::GetEnumSpriteIndexList(AZ::EntityId entityId, AZ::u32 indexMin, AZ::u32 indexMax, const char* errorMessage)
{
    AZu32ComboBoxVec indexStringComboVec;

    int indexCount = 0;
    UiIndexableImageBus::EventResult(indexCount, entityId, &UiIndexableImageBus::Events::GetImageIndexCount);
    const AZ::u32 indexCountu32 = static_cast<AZ::u32>(indexCount);

    if (indexCount > 0 && (indexMax <= indexCountu32 - 1) && indexMin <= indexMax)
    {
        for (AZ::u32 i = indexMin; i <= indexMax; ++i)
        {
            AZStd::string cellAlias;
            UiIndexableImageBus::EventResult(cellAlias, entityId, &UiIndexableImageBus::Events::GetImageIndexAlias, i);

            AZStd::string indexStrBuffer;

            if (cellAlias.empty())
            {
                indexStrBuffer = AZStd::string::format("%d", i);
            }
            else
            {
                indexStrBuffer = AZStd::string::format("%d (%s)", i, cellAlias.c_str());
            }

            indexStringComboVec.push_back(
                AZStd::make_pair(
                    i,
                    indexStrBuffer
                ));
        }
    }
    else
    {
        // Add an empty element to prevent an AzToolsFramework warning that fires
        // when an empty container is encountered.
        indexStringComboVec.push_back(AZStd::make_pair(0, errorMessage));
    }

    return indexStringComboVec;
}

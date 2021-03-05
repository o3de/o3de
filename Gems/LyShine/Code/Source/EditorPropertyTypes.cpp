/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "LyShine_precompiled.h"
#include "EditorPropertyTypes.h"

#include <LyShine/Bus/UiIndexableImageBus.h>

#include <AzCore/EBus/EBus.h>

LyShine::AZu32ComboBoxVec LyShine::GetEnumSpriteIndexList(AZ::EntityId entityId, AZ::u32 indexMin, AZ::u32 indexMax, const char* errorMessage)
{
    AZu32ComboBoxVec indexStringComboVec;

    int indexCount = 0;
    EBUS_EVENT_ID_RESULT(indexCount, entityId, UiIndexableImageBus, GetImageIndexCount);

    if (indexCount > 0 && (indexMax <= indexCount - 1) && indexMin <= indexMax)
    {
        for (AZ::u32 i = indexMin; i <= indexMax; ++i)
        {
            AZStd::string cellAlias;
            EBUS_EVENT_ID_RESULT(cellAlias, entityId, UiIndexableImageBus, GetImageIndexAlias, i);

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
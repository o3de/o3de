/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/SlotMetadata.h>

#include <ScriptCanvas/Data/BehaviorContextObject.h>

namespace ScriptCanvas
{
    void SlotMetadata::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SlotMetadata>()
                ->Field("m_slotId", &SlotMetadata::m_slotId)
                ->Field("m_dataType", &SlotMetadata::m_dataType)
                ;
        }
    }
}

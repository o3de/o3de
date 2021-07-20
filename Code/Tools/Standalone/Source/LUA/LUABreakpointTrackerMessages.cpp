/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "LUABreakpointTrackerMessages.h"


namespace LUAEditor
{
    void Breakpoint::Reflect(AZ::ReflectContext* reflection)
    {
        // reflect data for script, serialization, editing...
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Breakpoint>()
                ->Version(3)
                ->Field("m_breakpointId", &Breakpoint::m_breakpointId)
                ->Field("m_assetId", &Breakpoint::m_assetId)
                ->Field("m_documentLine", &Breakpoint::m_documentLine)
                ->Field("m_assetName", &Breakpoint::m_assetName)
            ;
        }
    }

    void Breakpoint::RepurposeToNewOwner(const AZStd::string& newAssetName, const AZStd::string& newAssetId)
    {
        m_assetName = newAssetName;
        m_assetId = newAssetId;
    }
}

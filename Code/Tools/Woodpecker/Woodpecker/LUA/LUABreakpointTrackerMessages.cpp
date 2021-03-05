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

#include "Woodpecker_precompiled.h"
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

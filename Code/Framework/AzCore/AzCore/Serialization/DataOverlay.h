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
#ifndef AZCORE_DATA_OVERLAY_H
#define AZCORE_DATA_OVERLAY_H

#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    typedef u32 DataOverlayProviderId;              // Intended to be a AZ_CRC

    class DataOverlayToken
    {
    public:
        AZ_TYPE_INFO(DataOverlayToken, "{21A62289-8838-42E4-A425-DCD72495E498}")
        AZStd::vector<u8>   m_dataUri;  // uri used by the provider to identify the data
    };

    /**
     * DataOverlayInfo contains overlay information, if any
     */
    struct DataOverlayInfo
    {
        AZ_TYPE_INFO(DataOverlayInfo, "{42DD05B6-19A8-4DEF-8718-8BF5CAC8828E}")
        DataOverlayInfo()
            : m_providerId(0) {}

        DataOverlayProviderId   m_providerId;   // Id of the provider responsible for this overlay.
        DataOverlayToken        m_dataToken;    // Provider-specific data used to identify and retrieve the actual data.
    };
}   // namespace AZ

#endif  // AZCORE_DATA_OVERLAY_H
#pragma once

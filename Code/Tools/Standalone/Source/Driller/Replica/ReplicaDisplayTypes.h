/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ReplicaDisplayTypes
{
    enum BandwidthUsageDisplayType
    {
        BUDT_START = -1,

        BUDT_COMBINED,
        BUDT_SENT,
        BUDT_RECEIVED,

        BUDT_END
    };

    class DisplayNames
    {
    public:
        static const char* BUDT_SENT_NAME;
        static const char* BUDT_RECEIVED_NAME;
        static const char* BUDT_COMBINED_NAME;
    };    
}

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
#pragma once
#ifndef WOODPECKER_PROFILER_REPLICA_REPLICADISPLAYTYPES_H
#define WOODPECKER_PROFILER_REPLICA_REPLICADISPLAYTYPES_H

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

#endif
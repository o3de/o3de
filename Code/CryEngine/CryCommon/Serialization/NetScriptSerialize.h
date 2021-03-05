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
#ifndef CRYINCLUDE_CRYCOMMON_CRYSCRIPTSYSTEM_NETSCRIPTSERIALIZE_H
#define CRYINCLUDE_CRYCOMMON_CRYSCRIPTSYSTEM_NETSCRIPTSERIALIZE_H

namespace Serialization
{
    class INetScriptMarshaler
    {
    public:
        virtual TSerialize FindSerializer(const char* name) = 0;
        virtual bool CommitSerializer(const char* name, TSerialize serializer) = 0;

        virtual int GetMaxServerProperties() const = 0;
    };
}

#endif
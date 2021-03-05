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

namespace AZ::SceneAPI::Utilities
{
    template <typename T>
    void DebugOutput::Write(const char* name, const AZStd::vector<T>& data)
    {
        m_output += AZStd::string::format("\t%s: Count %zu. Hash: %zu\n", name, data.size(), AZStd::hash_range(data.begin(), data.end()));
    }

    template <typename T>
    void DebugOutput::Write(const char* name, const AZStd::vector<AZStd::vector<T>>& data)
    {
        size_t hash = 0;

        for (auto&& vector : data)
        {
            AZStd::hash_combine(hash, AZStd::hash_range(vector.begin(), vector.end()));
        }

        m_output += AZStd::string::format("\t%s: Count %zu. Hash: %zu\n", name, data.size(), hash);
    }
}

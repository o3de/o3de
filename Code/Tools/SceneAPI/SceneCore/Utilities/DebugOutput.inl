/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::SceneAPI::Utilities
{
    template<typename T>
    void DebugOutput::Write(const char* name, const AZStd::vector<T>& data)
    {
        AZStd::size_t hash = AZStd::hash_range(data.begin(), data.end());
        AZ::u64 dataSize(static_cast<AZ::u64>(data.size()));
        m_output += AZStd::string::format("\t%s: Count %llu. Hash: %zu\n", name, dataSize, hash);

        AddToNode(AZStd::string::format("%s - Count", name).c_str(), dataSize);
        AddToNode(AZStd::string::format("%s - Hash", name).c_str(), static_cast<AZ::u64>(hash));
    }

    template<typename T>
    void DebugOutput::Write(const char* name, const AZStd::vector<AZStd::vector<T>>& data)
    {
        AZStd::size_t hash = 0;
        AZ::u64 dataSize(static_cast<AZ::u64>(data.size()));

        for (auto&& vector : data)
        {
            AZStd::hash_combine(hash, AZStd::hash_range(vector.begin(), vector.end()));
        }

        m_output += AZStd::string::format("\t%s: Count %llu. Hash: %zu\n", name, dataSize, hash);

        AddToNode(AZStd::string::format("%s - Count", name).c_str(), dataSize);
        AddToNode(AZStd::string::format("%s - Hash", name).c_str(), static_cast<AZ::u64>(hash));
    }

} // namespace AZ::SceneAPI::Utilities

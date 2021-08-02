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

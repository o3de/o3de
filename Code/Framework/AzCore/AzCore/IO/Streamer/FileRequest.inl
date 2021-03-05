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

#include <limits>

namespace AZ::IO
{
    template<typename T> T* FileRequest::GetCommandFromChain()
    {
        FileRequest* current = this;
        while (current)
        {
            if (T* command = AZStd::get_if<T>(&current->GetCommand()); command != nullptr)
            {
                return command;
            }
            current = current->m_parent;
        }
        return nullptr;
    }

    template<typename T> const T* FileRequest::GetCommandFromChain() const
    {
        const FileRequest* current = this;
        while (current)
        {
            if (const T* command = AZStd::get_if<T>(&current->GetCommand()); command != nullptr)
            {
                return command;
            }
            current = current->m_parent;
        }
        return nullptr;
    }

    constexpr size_t FileRequest::GetMaxNumDependencies()
    {
        // Leave a bit of room for requests that don't check the available space for dependencies
        // because they only add a small (usually one or two) number of dependencies.
        return std::numeric_limits<decltype(m_dependencies)>::max() - 255;
    }
} // namespace AZ::IO

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

#include <BufferedDataStream.h>

namespace Lumberyard
{
    BufferedDataStream::BufferedDataStream(uint32_t stream_type, const void* data, size_t data_size)
        : crashpad::MinidumpUserExtensionStreamDataSource(stream_type)
    {
        data_.resize(data_size);

        if (data_size)
        {
            memcpy(data_.data(), data, data_size);
        }
    }

    size_t BufferedDataStream::StreamDataSize()
    {
        return data_.size();
    }

    bool BufferedDataStream::ReadStreamData(Delegate* delegate)
    {
        return delegate->ExtensionStreamDataSourceRead(data_.size() ? data_.data() : nullptr, data_.size());
    }
}


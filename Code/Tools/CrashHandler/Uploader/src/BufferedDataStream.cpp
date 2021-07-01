/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BufferedDataStream.h>

namespace O3de
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


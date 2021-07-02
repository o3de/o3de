/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <minidump/minidump_user_extension_stream_data_source.h>
#include <vector>

namespace O3de
{
    class BufferedDataStream : public crashpad::MinidumpUserExtensionStreamDataSource
    {
    public:
        BufferedDataStream(uint32_t stream_type, const void* data, size_t data_size);

        size_t StreamDataSize() override;
        bool ReadStreamData(Delegate* delegate) override;

    private:
        std::vector<uint8_t> data_;

        BufferedDataStream(const BufferedDataStream& rhs) = delete;
        BufferedDataStream& operator=(const BufferedDataStream& rhs) = delete;
    };

}

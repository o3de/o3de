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

#include <minidump/minidump_user_extension_stream_data_source.h>
#include <vector>

namespace Lumberyard
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
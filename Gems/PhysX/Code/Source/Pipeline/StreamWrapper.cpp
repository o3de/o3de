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

#include <PhysX_precompiled.h>

#include <Pipeline/StreamWrapper.h>

namespace PhysX
{
    StreamWrapper::StreamWrapper(AZ::IO::GenericStream* stream)
        : m_stream(stream)
    {

    }

    uint32_t StreamWrapper::read(void* dest, uint32_t count)
    {
        return static_cast<uint32_t>(m_stream->Read(count, dest));
    }

    uint32_t StreamWrapper::write(const void* src, uint32_t count)
    {
        return static_cast<uint32_t>(m_stream->Write(count, src));
    }

    AssetDataStreamWrapper::AssetDataStreamWrapper(AZStd::shared_ptr<AZ::Data::AssetDataStream> stream)
        : m_stream(stream)
    {
    }

    uint32_t AssetDataStreamWrapper::read(void* dest, uint32_t count)
    {
        return static_cast<uint32_t>(m_stream->Read(count, dest));
    }

}

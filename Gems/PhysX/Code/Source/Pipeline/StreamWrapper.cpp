/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

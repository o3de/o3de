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

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <extensions/PxDefaultStreams.h>

namespace PhysX
{
    /// Wraps an AZ stream by provided the physx interface.
    /// This is used to prevent copying of data when going from
    /// physx streams to az streams.
    class StreamWrapper 
        : public physx::PxInputStream
        , public physx::PxOutputStream

    {
    public:
        StreamWrapper(AZ::IO::GenericStream* stream);

        uint32_t read(void* dest, uint32_t count) override;
        uint32_t write(const void* src, uint32_t count) override;

    private:
        AZ::IO::GenericStream* m_stream;
    };

    /// Wraps an AZ AssetDataStream read-only stream with the physx interface.
    /// This is used to prevent copying of data when going from
    /// physx streams to az streams.
    class AssetDataStreamWrapper
        : public physx::PxInputStream

    {
    public:
        AssetDataStreamWrapper(AZStd::shared_ptr<AZ::Data::AssetDataStream> stream);

        uint32_t read(void* dest, uint32_t count) override;

    private:
        AZStd::shared_ptr<AZ::Data::AssetDataStream> m_stream;
    };
}

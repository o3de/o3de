/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzNetworking/Framework/ICompressor.h>

namespace MultiplayerCompression
{
    class MultiplayerCompressionFactory
        : public AzNetworking::ICompressorFactory
    {
    public:
        //! Instantiate a new compressor
        //! @return A unique_ptr to a new Compressor
        AZStd::unique_ptr<AzNetworking::ICompressor> Create() override;

        //! Gets the AZ Name of this compressor factory
        //! @return the AZ Name of this compressor factory
        AZ::Name GetFactoryName() const override;

    private:
        const AZ::Name m_name = AZ::Name("MultiplayerCompressor");
    };
}

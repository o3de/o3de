/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/ranges/ranges_algorithm.h>
#include "CompressionFactoryImpl.h"

namespace Compression
{
    CompressionFactoryImpl::~CompressionFactoryImpl()
    {
        for (ICompressionInterface* compressionInterface : m_compressionInterfaces)
        {
            delete compressionInterface;
        }
    }
    AZStd::span<ICompressionInterface* const> CompressionFactoryImpl::GetCompressionInterfaces()
    {
        return m_compressionInterfaces;
    }

    bool CompressionFactoryImpl::RegisterCompressionInterface(AZStd::unique_ptr<ICompressionInterface>&& compressionInterface)
    {
        if (compressionInterface == nullptr)
        {
            return false;
        }
        auto FindCompression = [compressionAlgorithmId = compressionInterface->GetCompressionAlgorithmId()]
        (const ICompressionInterface* registeredInterface)
        {
            return registeredInterface->GetCompressionAlgorithmId() == compressionAlgorithmId;
        };
        if (auto compressionFoundIt = AZStd::ranges::find_if(m_compressionInterfaces, FindCompression);
            compressionFoundIt != m_compressionInterfaces.end())
        {
            return false;
        }

        // The Compression Interface isn't registered, so move it into the container
        m_compressionInterfaces.emplace_back(compressionInterface.release());
        return true;

    }

    bool CompressionFactoryImpl::UnregisterCompressionInterface(CompressionAlgorithmId compressionAlgorithmId)
    {
        auto FindCompression = [compressionAlgorithmId]
        (ICompressionInterface* registeredInterface)
        {
            if (registeredInterface->GetCompressionAlgorithmId() == compressionAlgorithmId)
            {
                delete registeredInterface;
                return true;
            }

            return false;
        };

        return AZStd::erase_if(m_compressionInterfaces, FindCompression) != 0;
    }

    ICompressionInterface* CompressionFactoryImpl::FindCompressionInterface(CompressionAlgorithmId compressionAlgorithmId) const
    {
        auto FindCompression = [compressionAlgorithmId]
        (const ICompressionInterface* registeredInterface)
        {
            return registeredInterface->GetCompressionAlgorithmId() == compressionAlgorithmId;
        };

        auto compressionFoundIt = AZStd::ranges::find_if(m_compressionInterfaces, FindCompression);
        return compressionFoundIt != m_compressionInterfaces.end() ? *compressionFoundIt : nullptr;
    }
}// namespace Compression

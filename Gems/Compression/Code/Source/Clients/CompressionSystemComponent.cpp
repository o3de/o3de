/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <Compression/CompressionLZ4API.h>
#include <Compression/CompressionTypeIds.h>
#include <Compression/DecompressionInterfaceAPI.h>
#include "DecompressorLZ4Impl.h"

#include <Clients/Streamer/DecompressorStackEntry.h>

namespace CompressionLZ4
{
    void RegisterDecompressorLZ4Interface()
    {
        // Register the lz4 decompressor with the decompression registrar
        if (auto decompressionRegistrar = Compression::DecompressionRegistrar::Get();
            decompressionRegistrar != nullptr)
        {
            auto compressionAlgorithmId = GetLZ4CompressionAlgorithmId();
            auto decompressorLz4 = AZStd::make_unique<DecompressorLZ4>();
            [[maybe_unused]] auto registerOutcome = decompressionRegistrar->RegisterDecompressionInterface(
                compressionAlgorithmId,
                AZStd::move(decompressorLz4));

            AZ_Error("Compression LZ4", bool{ registerOutcome }, "Registration of LZ4 Decompressor with the DecompressionRegistrar"
                " has failed with Id %u", compressionAlgorithmId);
        }
    }
    void UnregisterDecompressorLZ4Interface()
    {
        // Unregister the lz4 decompressor using the lz4 compression algorithm Id
        if (auto decompressionRegistrar = Compression::DecompressionRegistrar::Get();
            decompressionRegistrar != nullptr)
        {
            auto compressionAlgorithmId = GetLZ4CompressionAlgorithmId();
            [[maybe_unused]] bool unregisterOutcome = decompressionRegistrar->UnregisterDecompressionInterface(
                compressionAlgorithmId);

            AZ_Error("Compression LZ4", unregisterOutcome, "LZ4 Decompressor with Id %u is not registered with"
                " with DecompressionRegistrar", static_cast<AZ::u32>(compressionAlgorithmId));
        }
    }
}

namespace Compression
{
    AZ_COMPONENT_IMPL(CompressionSystemComponent, "CompressionSystemComponent",
        CompressionSystemComponentTypeId);

    void CompressionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect the Streamer DecompressionStackEntryConfig
        // to allow a DecompressionStackEntry to be loaded using JSON Serialization
        // from .setreg settings files
        DecompressorRegistrarConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CompressionSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void CompressionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CompressionService"));
    }

    void CompressionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("CompressionService"));
    }

    void CompressionSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void CompressionSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    CompressionSystemComponent::CompressionSystemComponent()
    {
        if (CompressionInterface::Get() == nullptr)
        {
            CompressionInterface::Register(this);
        }
    }

    CompressionSystemComponent::~CompressionSystemComponent()
    {
        if (CompressionInterface::Get() == this)
        {
            CompressionInterface::Unregister(this);
        }
    }

    void CompressionSystemComponent::Init()
    {
    }

    void CompressionSystemComponent::Activate()
    {
        CompressionRequestBus::Handler::BusConnect();
        CompressionLZ4::RegisterDecompressorLZ4Interface();
    }

    void CompressionSystemComponent::Deactivate()
    {
        CompressionLZ4::UnregisterDecompressorLZ4Interface();
        CompressionRequestBus::Handler::BusDisconnect();
    }

} // namespace Compression

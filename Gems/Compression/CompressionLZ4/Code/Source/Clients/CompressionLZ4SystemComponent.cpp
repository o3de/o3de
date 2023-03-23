/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "CompressionLZ4SystemComponent.h"

#include <CompressionLZ4/CompressionLZ4API.h>
#include <CompressionLZ4/CompressionLZ4TypeIds.h>
#include "DecompressorLZ4Impl.h"

#include <Compression/DecompressionInterfaceAPI.h>

namespace CompressionLZ4
{
    AZ_COMPONENT_IMPL(CompressionLZ4SystemComponent, "CompressionLZ4SystemComponent",
        CompressionLZ4EditorSystemComponentTypeId);

    void CompressionLZ4SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CompressionLZ4SystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void CompressionLZ4SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CompressionLZ4Service"));
    }

    void CompressionLZ4SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("CompressionLZ4Service"));
    }

    void CompressionLZ4SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void CompressionLZ4SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    CompressionLZ4SystemComponent::CompressionLZ4SystemComponent()
    {
        if (CompressionLZ4Interface::Get() == nullptr)
        {
            CompressionLZ4Interface::Register(this);
        }
    }

    CompressionLZ4SystemComponent::~CompressionLZ4SystemComponent()
    {
        if (CompressionLZ4Interface::Get() == this)
        {
            CompressionLZ4Interface::Unregister(this);
        }
    }

    void CompressionLZ4SystemComponent::Init()
    {
    }

    void CompressionLZ4SystemComponent::Activate()
    {
        CompressionLZ4RequestBus::Handler::BusConnect();

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

    void CompressionLZ4SystemComponent::Deactivate()
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
        CompressionLZ4RequestBus::Handler::BusDisconnect();
    }

} // namespace CompressionLZ4

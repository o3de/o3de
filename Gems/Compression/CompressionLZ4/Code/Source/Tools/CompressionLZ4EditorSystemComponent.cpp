/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "CompressionLZ4EditorSystemComponent.h"

#include <CompressionLZ4/CompressionLZ4API.h>
#include <CompressionLZ4/CompressionLZ4TypeIds.h>
#include "CompressorLZ4Impl.h"

#include <Compression/CompressionInterfaceAPI.h>

namespace CompressionLZ4
{
    AZ_COMPONENT_IMPL(CompressionLZ4EditorSystemComponent, "CompressionLZ4EditorSystemComponent",
        CompressionLZ4EditorSystemComponentTypeId, BaseSystemComponent);

    void CompressionLZ4EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CompressionLZ4EditorSystemComponent, CompressionLZ4SystemComponent>()
                ->Version(0);
        }
    }

    CompressionLZ4EditorSystemComponent::CompressionLZ4EditorSystemComponent() = default;

    CompressionLZ4EditorSystemComponent::~CompressionLZ4EditorSystemComponent() = default;

    void CompressionLZ4EditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("CompressionLZ4EditorService"));
    }

    void CompressionLZ4EditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("CompressionLZ4EditorService"));
    }

    void CompressionLZ4EditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void CompressionLZ4EditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void CompressionLZ4EditorSystemComponent::Activate()
    {
        CompressionLZ4SystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        // Register the lz4 compressor with the compression registrar
        if (auto compressionRegistrar = Compression::CompressionRegistrar::Get();
            compressionRegistrar != nullptr)
        {
            auto compressionAlgorithmId = GetLZ4CompressionAlgorithmId();
            auto compressorLz4 = AZStd::make_unique<CompressorLZ4>();
            [[maybe_unused]] auto registerOutcome = compressionRegistrar->RegisterCompressionInterface(
                compressionAlgorithmId,
                AZStd::move(compressorLz4));

            AZ_Error("Compression LZ4", bool{ registerOutcome }, "Registration of LZ4 Compressor with the CompressionRegistrar"
                " has failed with Id %u", compressionAlgorithmId);
        }
    }

    void CompressionLZ4EditorSystemComponent::Deactivate()
    {
        // Unregister the lz4 compressor using the lz4 compression algorithm Id
        if (auto compressionRegistrar = Compression::CompressionRegistrar::Get();
            compressionRegistrar != nullptr)
        {
            auto compressionAlgorithmId = GetLZ4CompressionAlgorithmId();
            [[maybe_unused]] bool unregisterOutcome = compressionRegistrar->UnregisterCompressionInterface(
                compressionAlgorithmId);

            AZ_Error("Compression LZ4", unregisterOutcome, "LZ4 Compressor with Id %u is not registered with"
                " with CompressionRegistrar", static_cast<AZ::u32>(compressionAlgorithmId));
        }

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        CompressionLZ4SystemComponent::Deactivate();
    }

} // namespace CompressionLZ4

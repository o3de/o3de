/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionEditorSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <Compression/CompressionLZ4API.h>
#include <Compression/CompressionTypeIds.h>
#include "CompressorLZ4Impl.h"

#include <Compression/CompressionInterfaceAPI.h>

namespace CompressionLZ4
{
    void RegisterCompressorLZ4Interface()
    {
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
    void UnregisterCompressorLZ4Interface()
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
    }
}

namespace Compression
{
    AZ_COMPONENT_IMPL(CompressionEditorSystemComponent, "CompressionEditorSystemComponent",
        CompressionEditorSystemComponentTypeId, BaseSystemComponent);

    void CompressionEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CompressionEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    CompressionEditorSystemComponent::CompressionEditorSystemComponent() = default;

    CompressionEditorSystemComponent::~CompressionEditorSystemComponent() = default;

    void CompressionEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("CompressionEditorService"));
    }

    void CompressionEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("CompressionEditorService"));
    }

    void CompressionEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void CompressionEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void CompressionEditorSystemComponent::Activate()
    {
        CompressionSystemComponent::Activate();
        CompressionLZ4::RegisterCompressorLZ4Interface();
    }

    void CompressionEditorSystemComponent::Deactivate()
    {
        CompressionLZ4::UnregisterCompressorLZ4Interface();
        CompressionSystemComponent::Deactivate();
    }

} // namespace Compression

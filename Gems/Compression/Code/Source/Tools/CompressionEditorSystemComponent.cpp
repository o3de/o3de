/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "CompressionEditorSystemComponent.h"

namespace Compression
{
    void CompressionEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CompressionEditorSystemComponent, CompressionSystemComponent>()
                ->Version(0);
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
    }

    void CompressionEditorSystemComponent::Deactivate()
    {
        CompressionSystemComponent::Deactivate();
    }

} // namespace Compression

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/GenerationComponent.h>

namespace AZ::SceneAPI::SceneCore
{
    void GenerationComponent::Activate()
    {
        ActivateBindings();
    }

    void GenerationComponent::Deactivate()
    {
        DeactivateBindings();
    }

    void GenerationComponent::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GenerationComponent, AZ::Component, Events::CallProcessorBinder>()->Version(1);
        }
    }
} // namespace AZ::SceneAPI::SceneCore

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

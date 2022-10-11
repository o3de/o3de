/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StarsSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <StarsAsset.h>
#include <StarsFeatureProcessor.h>

namespace AZ::Render
{
    void StarsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        StarsFeatureProcessor::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<StarsSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void StarsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("StarsSystemService"));
    }

    void StarsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("StarsSystemService"));
    }

    void StarsSystemComponent::Activate()
    {
        m_starsAssetHandler = aznew AZ::Render::StarsAssetHandler();
        m_starsAssetHandler->Register();
    }

    void StarsSystemComponent::Deactivate()
    {
        m_starsAssetHandler->Unregister();
        delete m_starsAssetHandler;
    }

} // namespace Stars

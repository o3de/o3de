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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Interface/Interface.h>

#include <BuilderComponent.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AZ
{
    namespace AtomBridge
    {
        void BuilderComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<BuilderComponent, AZ::Component>()
                    ->Version(0)
                    ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                ;
            }
        }

        BuilderComponent::BuilderComponent()
        {
            AZ::Interface<AzFramework::AtomActiveInterface>::Register(this);
        }

        BuilderComponent::~BuilderComponent()
        {
            AZ::Component::~Component();
            AZ::Interface<AzFramework::AtomActiveInterface>::Unregister(this);
        }

        void BuilderComponent::Activate()
        {
        }

        void BuilderComponent::Deactivate()
        {
        }
    } // namespace RPI
} // namespace AZ

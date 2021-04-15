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

#include <Source/Multiplayer_precompiled.h>
#include <Source/MultiplayerToolsModule.h>
#include "Pipeline/NetworkPrefabProcessor.h"
#include "AzCore/Serialization/Json/RegistrationContext.h"
#include "Prefab/Instance/InstanceSerializer.h"

namespace Multiplayer
{
    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerToolsSystemComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MultiplayerToolsSystemComponent, "{65AF5342-0ECE-423B-B646-AF55A122F72B}");

        static void Reflect(AZ::ReflectContext* context)
        {
            NetworkPrefabProcessor::Reflect(context);
        }

        MultiplayerToolsSystemComponent() = default;
        ~MultiplayerToolsSystemComponent() override = default;

        /// AZ::Component overrides.
        void Activate() override
        {

        }

        void Deactivate() override
        {

        }
    };

    MultiplayerToolsModule::MultiplayerToolsModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
                MultiplayerToolsSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList MultiplayerToolsModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<MultiplayerToolsSystemComponent>(),
        };
    }
} // namespace Multiplayer

AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer2_Tools, Multiplayer::MultiplayerToolsModule);

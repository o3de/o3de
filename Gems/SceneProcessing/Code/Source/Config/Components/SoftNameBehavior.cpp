/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Config/SceneProcessingConfigBus.h>
#include <Config/Components/SoftNameBehavior.h>
#include <Config/SettingsObjects/SoftNameSetting.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        void SoftNameBehavior::Activate()
        {
            SceneAPI::Events::GraphMetaInfoBus::Handler::BusConnect();
        }

        void SoftNameBehavior::Deactivate()
        {
            SceneAPI::Events::GraphMetaInfoBus::Handler::BusDisconnect();
        }

        void SoftNameBehavior::GetVirtualTypes(AZStd::set<Crc32>& types, const SceneAPI::Containers::Scene& scene,
            SceneAPI::Containers::SceneGraph::NodeIndex node)
        {
            const AZStd::vector<SoftNameSetting*>* softNames = nullptr;
            SceneProcessingConfigRequestBus::BroadcastResult(softNames, &SceneProcessingConfigRequestBus::Events::GetSoftNames);

            if (softNames)
            {
                for (const SoftNameSetting* softName : *softNames)
                {
                    if (types.find(softName->GetVirtualTypeHash()) != types.end())
                    {
                        // This type has already been added.
                        continue;
                    }

                    if (softName->IsVirtualType(scene, node))
                    {
                        types.insert(softName->GetVirtualTypeHash());
                    }
                }
            }
        }

        void SoftNameBehavior::GetVirtualTypeName(AZStd::string& name, Crc32 type)
        {
            if (type == AZ_CRC("Ignore", 0x0d88d6e2))
            {
                name = "Ignore";
            }
        }

        void SoftNameBehavior::GetAllVirtualTypes(AZStd::set<Crc32>& types)
        {
            // Add types that aren't handled by one specific behavior and have a more global utility.
            if (types.find(AZ_CRC("Ignore", 0x0d88d6e2)) == types.end())
            {
                types.insert(AZ_CRC("Ignore", 0x0d88d6e2));
            }
        }

        void SoftNameBehavior::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SoftNameBehavior, BehaviorComponent>()->Version(1);
            }
        }
    } // namespace SceneProcessingConfig
} // namespace AZ

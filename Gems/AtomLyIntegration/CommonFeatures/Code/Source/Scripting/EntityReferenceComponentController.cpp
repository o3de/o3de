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

#include <Scripting/EntityReferenceComponentController.h>

namespace AZ
{
    namespace Render
    {
        void EntityReferenceComponentController::Reflect(AZ::ReflectContext* context)
        {
            EntityReferenceComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EntityReferenceComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &EntityReferenceComponentController::m_configuration);
            }
        }

        void EntityReferenceComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("EntityReferenceService"));
        }

        void EntityReferenceComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EntityReferenceService"));
        }

        EntityReferenceComponentController::EntityReferenceComponentController(const EntityReferenceComponentConfig& config)
            : m_configuration(config)
        {

        }

        void EntityReferenceComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            EntityReferenceRequestBus::Handler::BusConnect(m_entityId);
        }

        void EntityReferenceComponentController::Deactivate()
        {
            EntityReferenceRequestBus::Handler::BusDisconnect(m_entityId);

            m_entityId.SetInvalid();
        }

        void EntityReferenceComponentController::SetConfiguration(const EntityReferenceComponentConfig& config)
        {
            m_configuration = config;
        }

        AZStd::vector<EntityId> EntityReferenceComponentController::GetEntityReferences() const
        {
            return m_configuration.m_entityIdReferences;
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AtomLyIntegration/CommonFeatures/Scripting/EntityReferenceRequestBus.h>
#include <AtomLyIntegration/CommonFeatures/Scripting/EntityReferenceComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class EntityReferenceComponentController final
            : public EntityReferenceRequestBus::Handler
        {
        public:
            friend class EditorEntityReferenceComponent;

            AZ_TYPE_INFO(AZ::Render::EntityReferenceComponentController, "{89D1D8DE-AC1F-4069-8884-5A04582C2EB1}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            EntityReferenceComponentController() = default;
            EntityReferenceComponentController(const EntityReferenceComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const EntityReferenceComponentConfig& config);
            const EntityReferenceComponentConfig& GetConfiguration() const { return m_configuration; }

            // EntityReferenceRequestBus Overrides
            virtual AZStd::vector<EntityId> GetEntityReferences() const override;
        private:
            AZ_DISABLE_COPY(EntityReferenceComponentController);

            EntityReferenceComponentConfig m_configuration;

            EntityId m_entityId;
        };
    }
}

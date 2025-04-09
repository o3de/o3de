/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file ModelTagSystemComponent.h
 * @brief Contains the definition of the ModelTagSystemComponent that will allow to set all texture tags
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RPI.Public/AssetTagBus.h>
#include <Atom/RPI.Public/Configuration.h>

namespace AZ
{
    namespace RPI
    {
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API ModelTagSystemComponent final
            : public AZ::Component
            , ModelTagBus::Handler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_COMPONENT(ModelTagSystemComponent, "{93D69578-C521-43BC-ADAE-230DB09B361C}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            ModelTagSystemComponent() = default;
            ~ModelTagSystemComponent() override;

            void Activate() override;
            void Deactivate() override;

            // ModelTagBus::Handler overrides
            AssetQuality GetQuality(const AZ::Name& modelTag) const override;

            AZStd::vector<AZ::Name> GetTags() const override;

            void RegisterAsset(AZ::Name modelTag, const Data::AssetId& assetId) override;
            void RegisterTag(AZ::Name modelTag) override;

            void SetQuality(const AZ::Name& modelTag, AssetQuality quality) override;

        private:
            ModelTagSystemComponent(const ModelTagSystemComponent&) = delete;

            struct TagData
            {
                AZ_TYPE_INFO(TagData, "{8E452207-BA4F-4A08-BFD8-8A23757B8BAD}");

                AssetQuality quality = AssetQualityHighest;
                AZStd::unordered_set<Data::AssetId> registeredModels;
            };

            AZStd::unordered_map<AZ::Name, TagData> m_modelTags;
        };
    } // namespace RPI
} // namespace AZ

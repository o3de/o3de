/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file TextureTagSystemComponent.h
 * @brief Contains the definition of the TextureTagSystemComponent that will allow to set all texture tags
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
        class ATOM_RPI_PUBLIC_API ImageTagSystemComponent final
            : public AZ::Component
            , ImageTagBus::Handler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_COMPONENT(ImageTagSystemComponent, "{1AC446A3-6F20-4D5C-9C09-BB034C9188D5}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            ImageTagSystemComponent() = default;
            ~ImageTagSystemComponent() override;

            void Activate() override;
            void Deactivate() override;

            // ImageTagBus::Handler overrides
            AssetQuality GetQuality(const AZ::Name& imageTag) const override;

            AZStd::vector<AZ::Name> GetTags() const override;

            void RegisterAsset(AZ::Name imageTag, const Data::AssetId& assetId) override;
            void RegisterTag(AZ::Name imageTag) override;

            void SetQuality(const AZ::Name& imageTag, AssetQuality quality) override;

        private:
            ImageTagSystemComponent(const ImageTagSystemComponent&) = delete;

            struct TagData
            {
                AZ_TYPE_INFO(TagData, "{CC8A5564-6A27-48A4-A143-C914C1AB50D5}");

                AssetQuality m_quality = AssetQualityHighest;
                AZStd::unordered_set<Data::AssetId> m_registeredImages;
            };

            AZStd::unordered_map<AZ::Name, TagData> m_imageTags;
        };
    } // namespace RPI
} // namespace AZ

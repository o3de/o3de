/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Decals/DecalComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class DecalRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(DecalRequests, "{E9FC84EC-C63A-4241-B284-B8B72487F269}");

            //! Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

            virtual ~DecalRequests() {}

            //! Gets the attenuation angle. This controls how much the angle between geometry and the decal affects decal opacity.
            virtual float GetAttenuationAngle() const = 0;

            //! Sets the attenuation angle. This controls how much the angle between geometry and the decal affects decal opacity.
            virtual void SetAttenuationAngle(float angle) = 0;

            //! Gets the decal opacity
            virtual float GetOpacity() const = 0;

            //! Sets the decal opacity
            virtual void SetOpacity(float opacity) = 0;

            //! Gets the decal color
            virtual const AZ::Vector3& GetDecalColor() const = 0;

            //! Sets the decal color
            virtual void SetDecalColor(const AZ::Vector3& color) = 0;

            //! Gets the decal color factor
            virtual float GetDecalColorFactor() const = 0;

            //! Sets the decal color factor
            virtual void SetDecalColorFactor(float colorFactor) = 0;

            //! Gets the decal normal map opacity
            virtual float GetNormalMapOpacity() const = 0;

            //! Sets the decal normal map opacity
            virtual void SetNormalMapOpacity(float opacity) = 0;

            //! Gets the decal sort key. Decals with a larger sort key appear over top of smaller sort keys.
            virtual uint8_t GetSortKey() const = 0;

            //! Sets the decal sort key. Decals with a larger sort key appear over top of smaller sort keys.
            virtual void SetSortKey(uint8_t sortKey) = 0;

            //! Sets the material asset Id for this decal
            virtual void SetMaterialAssetId(Data::AssetId) = 0;

            //! Gets the material assert Id for this decal
            virtual Data::AssetId GetMaterialAssetId() const = 0;
        };

        /// The EBus for requests to for setting and getting decal component properties.
        typedef AZ::EBus<DecalRequests> DecalRequestBus;

        class DecalNotifications
            : public ComponentBus
        {
        public:
            AZ_RTTI(DecalNotifications, "{BA81FBF5-FF66-4868-AD85-6B7954941B6B}");

            virtual ~DecalNotifications() {}

            //! Signals that the attenuation angle has changed.
            //! @param attenuationAngle This controls how much the angle between geometry and the decal affects decal opacity.
            virtual void OnAttenuationAngleChanged([[maybe_unused]] float attenuationAngle) { }

            //! Signals that the decal color has changed.
            //! @param decalColor This controls the decal color that gets multiplied with decal texture
            virtual void OnDecalColorChanged([[maybe_unused]] const AZ::Vector3& decalColor) { }

            //! Signals that the decal color factor has changed.
            //! @param decalColorFacor This controls the decal color factor (in this case it is a multiplier) applied to the decal color
            virtual void OnDecalColorFactorChanged([[maybe_unused]] float decalColorFacor) { }

            //! Signals that the opacity has changed.
            //! @param opacity The opaqueness of the decal.
            virtual void OnOpacityChanged([[maybe_unused]] float opacity){ }

            //! Signals that the opacity has changed.
            //! @param opacity The opaqueness of the decal.
            virtual void OnNormalMapOpacityChanged([[maybe_unused]] float opacity){ }

            //! Signals that the sortkey has changed.
            //! @param sortKey Decals with a larger sort key appear over top of smaller sort keys.
            virtual void OnSortKeyChanged([[maybe_unused]] uint8_t sortKey){ }

            //! Signals that the material has changed
            //! @param materialAsset The material asset of the decal
            virtual void OnMaterialChanged(Data::Asset<RPI::MaterialAsset> materialAsset){ }
        };

        /// The EBus for decal notification events.
        typedef AZ::EBus<DecalNotifications> DecalNotificationBus;
    } // namespace Render
} // namespace AZ

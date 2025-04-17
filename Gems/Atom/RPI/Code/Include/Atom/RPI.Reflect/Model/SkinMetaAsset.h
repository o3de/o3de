/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>

namespace AZ
{
    namespace RPI
    {
        //! The skin meta asset is an optional asset that belongs to a model.
        //! In case the model does not contain any mesh with skinning information, the skin meta asset won't be generated.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API SkinMetaAsset final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            static constexpr inline const char* DisplayName = "SkinMeta";
            static constexpr inline const char* Extension = "skinMeta";
            static constexpr inline const char* Group = "Model";

            AZ_RTTI(SkinMetaAsset, "{DEDBC099-A628-463F-81BB-47C45D1E1CB1}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(SkinMetaAsset, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            SkinMetaAsset() = default;
            ~SkinMetaAsset() = default;

            static constexpr uint32_t s_assetIdPrefix = 0x20000000;

            //! Construct the asset id for the skin meta asset based on the model asset id it belongs to.
            //! The generated asset id will embed the model asset id and use s_assetIdPrefix as sub-id.
            //! As there won't be more than exactly one skin meta asset, the sub-id is fixed.
            //! @param modelAssetId The asset id of the model this belongs to.
            //! @param modelAssetName The model asset or group name the meta asset belongs to.
            static AZ::Data::AssetId ConstructAssetId(const AZ::Data::AssetId& modelAssetId, const AZStd::string& modelAssetName);

            //! Set the asset status to ready.
            void SetReady();

            //! Set the mapping between joint names and its indices.
            //! @param jointNameToIndexMap Joint name to index mapping.
            void SetJointNameToIndexMap(const AZStd::unordered_map<AZStd::string, uint16_t>& jointNameToIndexMap);

            //! Get the mapping between joint names and its indices.
            const AZStd::unordered_map<AZStd::string, uint16_t>& GetJointNameToIndexMap() const;

            //! Get joint index by its name.
            //! @param jointName The joint name to retrieve the index for.
            //! @result The used joint index in the skin.
            static constexpr uint16_t InvalidJointIndex = 0xffff;
            uint16_t GetJointIndexByName(const AZStd::string& jointName) const;

        private:
            AZStd::unordered_map<AZStd::string, uint16_t> m_jointNameToIndexMap;
        };

        using SkinMetaAssetHandler = AssetHandler<SkinMetaAsset>;
    } // namespace RPI
} // namespace AZ

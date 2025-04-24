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
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ::RPI
{
    //! The morph target meta asset is an optional asset that belongs to a model.
    //! In case the model does not contain any morph targets, the meta asset won't be generated.
    AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    class ATOM_RPI_REFLECT_API MorphTargetMetaAsset final
        : public Data::AssetData
    {
        AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

    public:
        static constexpr inline const char* DisplayName = "MorphTargetMeta";
        static constexpr inline const char* Extension = "morphTargetMeta";
        static constexpr inline const char* Group = "Model";

        AZ_RTTI(MorphTargetMetaAsset, "{7F8D8CEA-6E03-4EA6-8E9B-3388D8E00EFD}", Data::AssetData);
        AZ_CLASS_ALLOCATOR(MorphTargetMetaAsset, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        MorphTargetMetaAsset() = default;
        ~MorphTargetMetaAsset() = default;

        //! Meta data for a morph target.
        struct MorphTarget
        {
            AZ_TYPE_INFO(MorphTargetMetaAsset::MorphTarget, "{13AA0784-26EE-48D4-9418-691B693716B1}")

            AZStd::string m_meshNodeName; /**< The name of the mesh we are referring to. One morph target may refer to multiple meshes.*/
            AZStd::string m_morphTargetName; /**< The name of the morph target we are referring to. */

            // !The index of the sub-mesh impacted by this morph target.
            // !There may be multiple MorphTarget objects which all refer to different meshes for the same animation, linked by m_morphTargetName.
            uint32_t m_meshIndex;

            //! All vertex deltas for all meshes and for all morph targets are stored in a giant buffer.
            //! This indicates the start index for the deform deltas for the given mesh and morph target.
            uint32_t m_startIndex;

            //! The number of deformed vertices for the given mesh and morph target.
            uint32_t m_numVertices;

            //! The minimum and maximum values used for position delta compression.
            //! The output position will be calculated by interpolating between the minimum and maximum value.
            float m_minPositionDelta; 
            float m_maxPositionDelta;

            //! Reference to the wrinkle mask, if it exists
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_wrinkleMask;

            static void Reflect(AZ::ReflectContext* context);
        };

        //! Construct the asset id for the morph target meta asset based on the model asset id it belongs to.
        //! The generated asset id will embed the model asset id and use s_assetIdPrefix as sub-id.
        //! As there won't be more than exactly one morph target meta asset, the sub-id is fixed.
        //! @param modelAssetId The asset id of the model this belongs to.
        //! @param modelAssetName The model asset or group name the meta asset belongs to.
        static AZ::Data::AssetId ConstructAssetId(const AZ::Data::AssetId& modelAssetId, const AZStd::string& modelAssetName);

        //! Set the asset status to ready.
        void SetReady();

        //! Add meta data for a morph target.
        //! @param morphTarget Meta target for the morph target.
        void AddMorphTarget(const MorphTarget& morphTarget);

        //! Get access to the morph target meta data.
        const AZStd::vector<MorphTarget>& GetMorphTargets() const;

    private:
        AZStd::vector<MorphTarget> m_morphTargets;

        static constexpr uint32_t s_assetIdPrefix = 0x30000000;
    };

    using MorphTargetMetaAssetHandler = AssetHandler<MorphTargetMetaAsset>;
} // namespace AZ::RPI

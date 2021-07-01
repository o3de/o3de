/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>

#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroupLayout;
    }

    namespace RPI
    {
        //! This asset defines the layout of a shader resource group, including any relevant metadata about reflected shader inputs.
        //! 
        //! This is an immutable, serialized asset. It can be either serialized-in or created dynamically using ShaderResourceGroupAssetCreator.
        //! See RPI::ShaderResourceGroup for runtime features based on this asset.
        class ShaderResourceGroupAsset final
            : public AZ::Data::AssetData
        {
            friend class ShaderResourceGroupAssetCreator;
            friend class ShaderResourceGroupAssetHandler;
            friend class ShaderResourceGroupAssetTester;

        public:
            AZ_RTTI(ShaderResourceGroupAsset, "{F8C9F4AE-3F6A-45AD-B4FB-0CA415FCC2E1}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupAsset, AZ::SystemAllocator, 0);

            static const char* DisplayName;
            static const char* Group;
            static const char* Extension;

            static void Reflect(ReflectContext* context);

            ShaderResourceGroupAsset() = default;

            AZ_DISABLE_COPY_MOVE(ShaderResourceGroupAsset);

            //! Returns the name of the ShaderResourceGroup, which is unique within the containing shader.
            const Name& GetName() const;

            //! Returns the layout that defines the low-level hardware layout for shader input bindings for the current API.
            const RHI::ShaderResourceGroupLayout* GetLayout() const;

            //! Returns the layout that defines the low-level hardware layout for shader input bindings for a specific API.
            const RHI::ShaderResourceGroupLayout* GetLayout(const RHI::APIType type) const;

            bool IsValid() const;

        private:
            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();

            bool FinalizeAfterLoad();

            //! Find the index in the m_perAPILayout for a API type.
            size_t FindAPITypeIndex(RHI::APIType type) const;

            //! The name ID of the SRG, unique within the parent shader.
            Name m_name;

            //! The layout of the SRG
            using APIShaderResourceGroupLayout = AZStd::pair<RHI::APIType, RHI::Ptr<RHI::ShaderResourceGroupLayout>>;
            AZStd::vector<APIShaderResourceGroupLayout> m_perAPILayout;

            static constexpr const size_t InvalidAPITypeIndex = std::numeric_limits<size_t>::max();

            //! Index that indicates which ShaderResourceGroupLayout to use.
            size_t m_currentAPITypeIndex = InvalidAPITypeIndex;
        };

        //! Asset handler for the Shader Resource Group asset.
        class ShaderResourceGroupAssetHandler final
            : public AssetHandler<ShaderResourceGroupAsset>
        {
            using Base = AssetHandler<ShaderResourceGroupAsset>;
        public:
            ShaderResourceGroupAssetHandler() = default;

        private:
            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;
            Data::AssetHandler::LoadResult PostLoadInit(const Data::Asset<Data::AssetData>& asset);
        };

    } // namespace RPI
} // namespace AZ

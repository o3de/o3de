/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Material/MaterialInstanceHandler.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {

        class Material;

        //! Manages system-wide initialization and support for material classes
        class MaterialSystem
            : public MaterialInstanceHandlerInterface::Registrar
            , public Data::AssetBus::Handler
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            // MaterialInstanceHandlerInterface
            MaterialInstanceData RegisterMaterialInstance(const Data::Instance<Material> material) override;
            void ReleaseMaterialInstance(const MaterialInstanceData& materialInstance) override;
#ifndef USE_BINDLESS_SRG
            int32_t RegisterMaterialTexture(
                const int materialTypeIndex, const int materialInstanceIndex, const Data::Instance<Image>& image) override;
#endif
            void Compile() override;

            void DebugPrintMaterialInstances();

            void Init();
            void Shutdown();

        private:
            bool LoadMaterialSrgShaderAsset();
            void CreateSceneMaterialSrg();
            void UpdateSceneMaterialSrg();
            void PrepareMaterialParameterBuffers();
            void UpdateChangedMaterialParameters();

            //  Data::AssetBus Interface
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            using MaterialIndexAllocator = PersistentIndexAllocator<int32_t>;

            struct InternalMaterialInstanceData
            {
                // either the sceneMaterialSRG, or a separate MaterialSrg for this Material-Instance only
                Data::Instance<ShaderResourceGroup> m_shaderResourceGroup;
#ifndef USE_BINDLESS_SRG
                AZStd::vector<Data::Instance<Image>> m_materialTextures;
                AZStd::unordered_map<Data::AssetId, int32_t> m_materialTexturesMap;
                bool m_materialTexturesDirty = false;
#endif
                Data::Instance<MaterialShaderParameter> m_shaderParameter;
                Material* m_material; // can't use a smart pointer here, since the material de-registers itself in the destructor
                size_t m_compiledChangeId;
            };

            struct MaterialTypeData
            {
                bool m_valid = false;
                bool m_useSceneMaterialSrg = false;
                Data::AssetId m_materialTypeAssetId;
                AZStd::string m_materialTypeAssetHint;
                MaterialIndexAllocator m_instanceIndices;
                Data::Instance<Buffer> m_parameterBuffer;
                AZStd::unordered_map<int, uint32_t> m_bindlessReadIndices;

                // we need our own 'raw' BufferView for the parameterBuffer so we can access it with the Bindless-SRG
                Data::Instance<RHI::BufferView> m_parameterBufferView;
                AZStd::unique_ptr<MaterialShaderParameterLayout> m_shaderParameterLayout;
                AZStd::vector<InternalMaterialInstanceData> m_instanceData;
            };

            MaterialIndexAllocator m_materialTypeIndices;
            AZStd::vector<MaterialTypeData> m_materialTypeData;
            AZStd::unordered_map<Data::AssetId, int32_t> m_materialTypeIndicesMap;

            RHI::ShaderInputNameIndex m_materialTypeBufferInputIndex = { "m_materialTypeBufferIndices" };
            Data::Asset<ShaderAsset> m_sceneMaterialSrgShaderAsset;
            Data::Instance<ShaderResourceGroup> m_sceneMaterialSrg;

            Data::Instance<Buffer> m_materialTypeBufferIndicesBuffer;
            bool m_bufferReadIndicesDirty = false;
        };

    } // namespace RPI
} // namespace AZ

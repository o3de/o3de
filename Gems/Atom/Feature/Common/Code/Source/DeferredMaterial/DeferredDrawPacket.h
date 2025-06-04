/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ
{
    namespace Render
    {

        // Forward declaration
        class DeferredDrawPacketManager;
        // This is a drawpacket with a single fullscreen draw item for one material-type and it's unique set of shader options
        class DeferredDrawPacket
            : public AZStd::intrusive_base
            , private RPI::ShaderReloadNotificationBus::Handler
        {
        public:
            DeferredDrawPacket() = default;
            DeferredDrawPacket(
                DeferredDrawPacketManager* drawPacketManager,
                const RPI::Scene* scene,
                RPI::Material* material,
                const Name& materialPipelineName,
                const RPI::ShaderCollection::Item& shaderItem,
                const int32_t drawPacketId);

            void CompileDrawSrg(Data::Instance<RPI::Buffer> drawPacketIdBuffer);

            const RHI::DrawPacket* GetRHIDrawPacket()
            {
                return m_drawPacket.get();
            }
            const RHI::DrawPacket* GetRHIDrawPacket() const
            {
                return m_drawPacket.get();
            }
            const RHI::ConstPtr<RHI::ConstantsLayout> GetRootConstantsLayout() const
            {
                return m_rootConstantsLayout;
            }

            RHI::DrawListTag GetDrawListTag() const
            {
                return m_drawListTag;
            }

            int32_t GetDrawPacketId() const
            {
                return m_drawPacketId;
            }

            size_t GetUseCount() const
            {
                return use_count();
            }

            bool NeedsRebuild() const
            {
                return m_needsRebuild;
            }

            RPI::ShaderVariantId GetShaderVariantId() const
            {
                return m_shaderVariantId;
            }

#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
            const Data::Asset<RPI::MaterialAsset>& GetInstigatingMaterialAsset() const
            {
                return m_instigatingMaterialAsset;
            }
            const Data::Asset<RPI::MaterialTypeAsset>& GetInstigatingMaterialTypeAsset() const
            {
                return m_instigatingMaterialTypeAsset;
            }
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */

            // TODO: Who sets shader-options for what?
            // We can only draw multiple meshes with one deferred draw-call if they have the same shader-options,
            // so if these shader-options are somehow linked to a mesh, that mesh needs a different draw-packet.
            // But if these are global (e.g. DebugRendering), this might still be useful
            // bool SetShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value);
            // bool UnsetShaderOption(const Name& shaderOptionName);
            // void ClearShaderOptions();

        private:
            void Init(
                const RPI::Scene* scene,
                RPI::Material* material,
                const Name& MaterialPipelineName,
                const RPI::ShaderCollection::Item& shaderItem);

            DeferredDrawPacketManager* m_drawPacketManager;

            // unique Id of the draw-packet
            int32_t m_drawPacketId;

#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
            // Non-valid Reference to the material-Asset that was used to create this DeferredDrawPacket.
            // Useful for debugging / logprints, but this should never be used to load the asset
            Data::Asset<RPI::MaterialAsset> m_instigatingMaterialAsset;
            Data::Asset<RPI::MaterialTypeAsset> m_instigatingMaterialTypeAsset;
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */

            Data::Instance<RPI::Shader> m_shader;
            RPI::ShaderVariantId m_shaderVariantId;
            RPI::ShaderOptionGroup m_shaderOptions;
            Name m_materialPipelineName;
            Name m_shaderTag;
            RHI::DrawListTag m_drawListTag;
            const RHI::PipelineState* m_pipelineState;
            Data::Instance<RPI::ShaderResourceGroup> m_materialSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_drawSrg;
            RHI::ConstPtr<RHI::ConstantsLayout> m_rootConstantsLayout;
            AZStd::vector<uint8_t> m_rootConstants;
            AZStd::shared_ptr<RHI::GeometryView> m_geometryView;

            RHI::ConstPtr<RHI::DrawPacket> m_drawPacket;

            bool m_needsRebuild = false;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant) override;
        };

    } // namespace Render
} // namespace AZ
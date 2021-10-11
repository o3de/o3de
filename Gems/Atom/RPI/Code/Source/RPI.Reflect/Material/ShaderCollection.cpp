/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/std/containers/vector_set.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DrawListTagRegistry.h>

namespace AZ
{
    namespace RPI
    {
        //! This allows ShaderCollection::Item to serialize only a ShaderVariantId rather than the ShaderOptionsGroup object,
        //! but still provide the corresponding ShaderOptionsGroup for use at runtime.
        //! RenderStates will be modified at runtime as well. It will be merged into the RenderStates stored in the corresponding ShaderVariant.
        class ShaderVariantReferenceSerializationEvents
            : public SerializeContext::IEventHandler
        {
            //! Called right before we start reading from the instance pointed by classPtr.
            void OnReadBegin(void* classPtr) override
            {
                ShaderCollection::Item* shaderVariantReference = reinterpret_cast<ShaderCollection::Item*>(classPtr);
                shaderVariantReference->m_shaderVariantId = shaderVariantReference->m_shaderOptionGroup.GetShaderVariantId();
            }

            //! Called right after we finish writing data to the instance pointed at by classPtr.
            void OnWriteEnd(void* classPtr) override
            {
                ShaderCollection::Item* shaderVariantReference = reinterpret_cast<ShaderCollection::Item*>(classPtr);

                // Dependent asset references aren't guaranteed to finish loading by the time this asset is serialized, only by
                // the time this asset load is completed.  But since the data is needed here, we will deliberately block until the
                // shader asset has finished loading.
                if (shaderVariantReference->m_shaderAsset.QueueLoad())
                {
                    shaderVariantReference->m_shaderAsset.BlockUntilLoadComplete();
                }

                if (shaderVariantReference->m_shaderAsset.IsReady())
                {
                    shaderVariantReference->m_shaderOptionGroup = ShaderOptionGroup{
                        shaderVariantReference->m_shaderAsset->GetShaderOptionGroupLayout(),
                        shaderVariantReference->m_shaderVariantId
                    };
                }
                else
                {
                    shaderVariantReference->m_shaderOptionGroup = {};
                }
            }
        };

        void ShaderCollection::Reflect(AZ::ReflectContext* context)
        {
            ShaderCollection::Item::Reflect(context);
            NameReflectionMapForIndex::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ShaderCollection>()
                    ->Version(5)
                    ->Field("ShaderItems", &ShaderCollection::m_shaderItems)
                    ->Field("ShaderTagIndexMap", &ShaderCollection::m_shaderTagIndexMap)
                    ;
            }
        }

        void ShaderCollection::Item::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ShaderCollection::Item>()
                    ->Version(6)
                    ->EventHandler<ShaderVariantReferenceSerializationEvents>()
                    ->Field("ShaderAsset", &ShaderCollection::Item::m_shaderAsset)
                    ->Field("ShaderVariantId", &ShaderCollection::Item::m_shaderVariantId)
                    ->Field("Enabled", &ShaderCollection::Item::m_enabled)
                    ->Field("OwnedShaderOptionIndices", &ShaderCollection::Item::m_ownedShaderOptionIndices)
                    ->Field("ShaderTag", &ShaderCollection::Item::m_shaderTag)
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<Item>("ShaderCollectionItem")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ->Method("GetShaderAsset", &Item::GetShaderAsset)
                    ->Method("GetShaderAssetId", &Item::GetShaderAssetId)
                    ->Method("GetShaderVariantId", &Item::GetShaderVariantId)
                    ->Method("GetShaderOptionGroup", &Item::GetShaderOptionGroup)
                ;
            }
        }

        size_t ShaderCollection::size() const
        {
            return m_shaderItems.size();
        }

        ShaderCollection::iterator ShaderCollection::begin()
        {
            return m_shaderItems.begin();
        }

        ShaderCollection::const_iterator ShaderCollection::begin() const
        {
            return m_shaderItems.begin();
        }

        ShaderCollection::iterator ShaderCollection::end()
        {
            return m_shaderItems.end();
        }

        ShaderCollection::const_iterator ShaderCollection::end() const
        {
            return m_shaderItems.end();
        }

        ShaderCollection::Item::Item()
        {
            m_renderStatesOverlay = RHI::GetInvalidRenderStates();
        }

        ShaderCollection::Item& ShaderCollection::operator[](size_t i)
        {
            return m_shaderItems[i];
        }

        const ShaderCollection::Item& ShaderCollection::operator[](size_t i) const
        {
            return m_shaderItems[i];
        }

        ShaderCollection::Item& ShaderCollection::operator[](const AZ::Name& shaderTag)
        {
            return m_shaderItems[m_shaderTagIndexMap.Find(shaderTag).GetIndex()];
        }

        const ShaderCollection::Item& ShaderCollection::operator[](const AZ::Name& shaderTag) const
        {
            return m_shaderItems[m_shaderTagIndexMap.Find(shaderTag).GetIndex()];
        }

        bool ShaderCollection::HasShaderTag(const AZ::Name& shaderTag) const
        {
            return (m_shaderTagIndexMap.Find(shaderTag).IsValid());
        }

        ShaderCollection::Item::Item(const Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& shaderTag, ShaderVariantId variantId)
            : m_shaderAsset(shaderAsset)
            , m_shaderVariantId(variantId)
            , m_shaderTag(shaderTag)
            , m_shaderOptionGroup(shaderAsset->GetShaderOptionGroupLayout(), variantId)
        {
        }

        ShaderCollection::Item::Item(Data::Asset<ShaderAsset>&& shaderAsset, const AZ::Name& shaderTag, ShaderVariantId variantId)
            : m_shaderAsset(AZStd::move(shaderAsset))
            , m_shaderVariantId(variantId)
            , m_shaderTag(shaderTag)
            , m_shaderOptionGroup(shaderAsset->GetShaderOptionGroupLayout(), variantId)
        {
        }

        const Data::Asset<ShaderAsset>& ShaderCollection::Item::GetShaderAsset() const
        {
            return m_shaderAsset;
        }

        const ShaderVariantId& ShaderCollection::Item::GetShaderVariantId() const
        {
            return m_shaderOptionGroup.GetShaderVariantId();
        }

        const ShaderOptionGroup* ShaderCollection::Item::GetShaderOptions() const
        {
            return &m_shaderOptionGroup;
        }

        ShaderOptionGroup* ShaderCollection::Item::GetShaderOptions()
        {
            return &m_shaderOptionGroup;
        }

        bool ShaderCollection::Item::MaterialOwnsShaderOption(const AZ::Name& shaderOptionName) const
        {
            return m_ownedShaderOptionIndices.contains(m_shaderOptionGroup.FindShaderOptionIndex(shaderOptionName));
        }

        bool ShaderCollection::Item::MaterialOwnsShaderOption(ShaderOptionIndex shaderOptionIndex) const
        {
            return m_ownedShaderOptionIndices.contains(shaderOptionIndex);
        }

        const RHI::RenderStates* ShaderCollection::Item::GetRenderStatesOverlay() const
        {
            return &m_renderStatesOverlay;
        }

        RHI::RenderStates* ShaderCollection::Item::GetRenderStatesOverlay()
        {
            return &m_renderStatesOverlay;
        }

        RHI::DrawListTag ShaderCollection::Item::GetDrawListTagOverride() const
        {
            return m_drawListTagOverride;
        }

        void ShaderCollection::Item::SetDrawListTagOverride(RHI::DrawListTag drawList)
        {
            m_drawListTagOverride = drawList;
        }

        void ShaderCollection::Item::SetDrawListTagOverride(const AZ::Name& drawListName)
        {
            if (drawListName.IsEmpty())
            {
                m_drawListTagOverride.Reset();
                return;
            }

            RHI::DrawListTagRegistry* drawListTagRegistry = RHI::RHISystemInterface::Get()->GetDrawListTagRegistry();
            // Note: we should use FindTag instead of AcquireTag to avoid occupy DrawListTag entries. 
            RHI::DrawListTag newTag = drawListTagRegistry->FindTag(drawListName);
            if (newTag.IsNull())
            {
                AZ_Error("ShaderCollection", false, "Failed to set draw list with name: %s.", drawListName.GetCStr());
                return;
            }
            m_drawListTagOverride = newTag;
        }

        void ShaderCollection::Item::SetEnabled(bool enabled)
        {
            m_enabled = enabled;
        }

        bool ShaderCollection::Item::IsEnabled() const
        {
            return m_enabled;
        }

        const AZ::Name& ShaderCollection::Item::GetShaderTag() const
        {
            return m_shaderTag;
        }

        const Data::AssetId& ShaderCollection::Item::GetShaderAssetId() const
        {
            return m_shaderAsset->GetId();
        }

        const AZ::RPI::ShaderOptionGroup& ShaderCollection::Item::GetShaderOptionGroup() const
        {
            return m_shaderOptionGroup;
        }

    } // namespace RPI
} // namespace AZ

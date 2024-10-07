/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptAsset.h>

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPipelineSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertySourceData.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataHolder.h>
#include <Atom/RPI.Edit/Material/LuaMaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Edit/Common/AssetAliasesSourceData.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAsset.h>
#include <Atom/RPI.Reflect/Model/SkinMetaAsset.h>
#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantTreeAsset.h>

#include <BuilderComponent.h>
#include <Common/AnyAssetBuilder.h>
#include <Material/MaterialBuilder.h>
#include <Material/MaterialPipelineScriptRunner.h>
#include <Material/MaterialTypeBuilder.h>
#include <ResourcePool/ResourcePoolBuilder.h>
#include <Pass/PassBuilder.h>

namespace AZ
{
    namespace RPI
    {
        void BuilderComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<BuilderComponent, AZ::Component>()
                    ->Version(0)
                    ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                ;
            }

            MaterialPropertySourceData::Reflect(context);
            MaterialTypeSourceData::Reflect(context);
            MaterialSourceData::Reflect(context);
            MaterialPipelineSourceData::Reflect(context);
            MaterialPropertyValueSourceData::Reflect(context);
            MaterialFunctorSourceData::Reflect(context);
            MaterialFunctorSourceDataHolder::Reflect(context);
            LuaMaterialFunctorSourceData::Reflect(context);
            ResourcePoolSourceData::Reflect(context);
            ConvertibleSource::Reflect(context);
            AssetAliasesSourceData::Reflect(context);
            ShaderSourceData::Reflect(context);
            ShaderVariantListSourceData::Reflect(context);
            MaterialPipelineScriptRunner::Reflect(context);
        }

        BuilderComponent::BuilderComponent()
        {
            m_materialFunctorRegistration.Init();
        }

        BuilderComponent::~BuilderComponent()
        {
            m_materialFunctorRegistration.Shutdown();
        }

        void BuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("RPIBuilderService"));
        }

        void BuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("ScriptService"));
        }

        void BuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("RPIBuilderService"));
        }

        void BuilderComponent::Activate()
        {
            // Register asset workers
            m_assetWorkers.emplace_back(MakeAssetBuilder<MaterialBuilder>());
            m_assetWorkers.emplace_back(MakeAssetBuilder<MaterialTypeBuilder>());
            m_assetWorkers.emplace_back(MakeAssetBuilder<ResourcePoolBuilder>());
            m_assetWorkers.emplace_back(MakeAssetBuilder<AnyAssetBuilder>());
            m_assetWorkers.emplace_back(MakeAssetBuilder<PassBuilder>());

            m_assetHandlers.emplace_back(MakeAssetHandler<ShaderAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<MaterialTypeAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<MaterialAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<ResourcePoolAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<StreamingImagePoolAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<BufferAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<ModelLodAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<ModelAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<PassAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<ShaderVariantAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<ShaderVariantTreeAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<SkinMetaAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<MorphTargetMetaAssetHandler>());
        }

        void BuilderComponent::Deactivate()
        {
            m_assetHandlers.clear();
            m_assetWorkers.clear();
        }
    } // namespace RPI
} // namespace AZ

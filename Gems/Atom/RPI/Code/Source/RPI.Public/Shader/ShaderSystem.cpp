/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/ShaderSystem.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroupPool.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantTreeAsset.h>
#include <Atom/RPI.Reflect/Shader/PrecompiledShaderAssetSourceData.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        static constexpr char ShaderSystemLog[] = "ShaderSystem";

        void ShaderSystem::Reflect(ReflectContext* context)
        {
            ShaderResourceGroupAsset::Reflect(context);
            ShaderOptionDescriptor::Reflect(context);
            ShaderOptionGroupLayout::Reflect(context);
            ShaderOptionGroupHints::Reflect(context);
            ShaderOptionGroup::Reflect(context);
            ShaderVariantId::Reflect(context);
            ShaderVariantStableId::Reflect(context);
            ShaderAsset::Reflect(context);
            ShaderInputContract::Reflect(context);
            ShaderOutputContract::Reflect(context);
            ShaderVariantAsset::Reflect(context);
            ShaderVariantTreeAsset::Reflect(context);
            ReflectShaderStageType(context);
            PrecompiledShaderAssetSourceData::Reflect(context);
        }

        ShaderSystemInterface* ShaderSystemInterface::Get()
        {
            return Interface<ShaderSystemInterface>::Get();
        }

        void ShaderSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<ShaderAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ShaderResourceGroupAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ShaderVariantAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ShaderVariantTreeAssetHandler>());
        }

        void ShaderSystem::Init()
        {
            m_shaderVariantAsyncLoader.Init();

            Interface<ShaderSystemInterface>::Register(this);

            {
                Data::InstanceHandler<Shader> handler;
                handler.m_createFunction = [](Data::AssetData* shaderAsset)
                {
                    return Shader::CreateInternal(*(azrtti_cast<ShaderAsset*>(shaderAsset)));
                };
                Data::InstanceDatabase<Shader>::Create(azrtti_typeid<ShaderAsset>(), handler);
            }

            {
                Data::InstanceHandler<ShaderResourceGroup> handler;
                handler.m_createFunction = [](Data::AssetData* srgAsset)
                {
                    return ShaderResourceGroup::CreateInternal(*(azrtti_cast<ShaderResourceGroupAsset*>(srgAsset)));
                };
                Data::InstanceDatabase<ShaderResourceGroup>::Create(azrtti_typeid<ShaderResourceGroupAsset>(), handler);
            }

            {
                Data::InstanceHandler<ShaderResourceGroupPool> handler;
                handler.m_createFunction = [](Data::AssetData* srgAsset)
                {
                    return ShaderResourceGroupPool::CreateInternal(*(azrtti_cast<ShaderResourceGroupAsset*>(srgAsset)));
                };
                Data::InstanceDatabase<ShaderResourceGroupPool>::Create(azrtti_typeid<ShaderResourceGroupAsset>(), handler);
            }
        }

        void ShaderSystem::Shutdown()
        {
            Data::InstanceDatabase<Shader>::Destroy();
            Data::InstanceDatabase<ShaderResourceGroup>::Destroy();
            Data::InstanceDatabase<ShaderResourceGroupPool>::Destroy();
            Interface<ShaderSystemInterface>::Unregister(this);
            m_shaderVariantAsyncLoader.Shutdown();
        }


        ///////////////////////////////////////////////////////////////////
        // ShaderSystemInterface overrides
        void ShaderSystem::SetGlobalShaderOption(const AZ::Name& shaderOptionName, ShaderOptionValue value)
        {
            bool changed = false;

            auto iter = m_globalShaderOptionValues.find(shaderOptionName);
            if (iter == m_globalShaderOptionValues.end())
            {
                changed = true;
                m_globalShaderOptionValues[shaderOptionName] = value;
            }
            else if (iter->second != value)
            {
                iter->second = value;
                changed = true;
            }

            if (changed)
            {
                m_globalShaderOptionUpdatedEvent.Signal(shaderOptionName, AZStd::move(value));
            }
        }

        ShaderOptionValue ShaderSystem::GetGlobalShaderOption(const AZ::Name& shaderOptionName)
        {
            ShaderOptionValue value;
            auto iter = m_globalShaderOptionValues.find(shaderOptionName);
            if (iter != m_globalShaderOptionValues.end())
            {
                value = iter->second;
            }
            return value;
        }

        const ShaderSystem::GlobalShaderOptionMap& ShaderSystem::GetGlobalShaderOptions() const
        {
            return m_globalShaderOptionValues;
        }

        void ShaderSystem::Connect(GlobalShaderOptionUpdatedEvent::Handler& handler)
        {
            handler.Connect(m_globalShaderOptionUpdatedEvent);
        }
        ///////////////////////////////////////////////////////////////////

    } // namespace RPI
} // namespace AZ

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <Asset/RuntimeAssetSystemComponent.h>
#include <Asset/SubgraphInterfaceAssetHandler.h>
#include <Execution/ExecutionState.h>
#include <ScriptCanvas/Execution/Executor.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>

namespace ScriptCanvas
{
    RuntimeAssetSystemComponent::~RuntimeAssetSystemComponent()
    {
    }

    void RuntimeAssetSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        RuntimeData::Reflect(context);
        RuntimeDataOverrides::Reflect(context);
        SubgraphInterfaceData::Reflect(context);
        Executor::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeAssetSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void RuntimeAssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptCanvasRuntimeAssetService"));
    }

    void RuntimeAssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AssetDatabaseService"));
        required.push_back(AZ_CRC_CE("ScriptCanvasService"));
    }

    void RuntimeAssetSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    void RuntimeAssetSystemComponent::Init()
    {
    }

    void RuntimeAssetSystemComponent::Activate()
    {
        m_runtimeAssetRegistry.Register<ScriptCanvas::RuntimeAsset, ScriptCanvas::RuntimeAssetHandler, ScriptCanvas::RuntimeAssetDescription>();
        m_runtimeAssetRegistry.Register<ScriptCanvas::SubgraphInterfaceAsset, ScriptCanvas::SubgraphInterfaceAssetHandler, ScriptCanvas::SubgraphInterfaceAssetDescription>();
    }

    void RuntimeAssetSystemComponent::Deactivate()
    {
        m_runtimeAssetRegistry.Unregister();
    }

    AssetRegistry& RuntimeAssetSystemComponent::GetAssetRegistry()
    {
        return m_runtimeAssetRegistry;
    }

}

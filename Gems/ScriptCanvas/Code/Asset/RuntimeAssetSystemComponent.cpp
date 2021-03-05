/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <Asset/RuntimeAssetSystemComponent.h>
#include <Asset/Functions/RuntimeFunctionAssetHandler.h>

namespace ScriptCanvas
{
    RuntimeAssetSystemComponent::~RuntimeAssetSystemComponent()
    {
    }

    void RuntimeAssetSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ScriptCanvas::RuntimeData::Reflect(context);
        ScriptCanvas::FunctionRuntimeData::Reflect(context);
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeAssetSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void RuntimeAssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasRuntimeAssetService", 0x1a85bf2b));
    }

    void RuntimeAssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
    }

    void RuntimeAssetSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void RuntimeAssetSystemComponent::Init()
    {
    }

    void RuntimeAssetSystemComponent::Activate()
    {
        m_runtimeAssetRegistry.Register<ScriptCanvas::RuntimeAsset, ScriptCanvas::RuntimeAssetHandler, ScriptCanvas::RuntimeAssetDescription>();
        m_runtimeAssetRegistry.Register<ScriptCanvas::RuntimeFunctionAsset, ScriptCanvas::RuntimeFunctionAssetHandler, ScriptCanvas::RuntimeFunctionAssetDescription>();
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

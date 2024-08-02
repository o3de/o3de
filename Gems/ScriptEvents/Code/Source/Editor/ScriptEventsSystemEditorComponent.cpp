/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventsSystemEditorComponent.h"

#include <ScriptEvents/Internal/VersionedProperty.h>

#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>
#include <ScriptEvents/ScriptEventDefinition.h>

#include <ScriptEvents/ScriptEvent.h>
#include <AzCore/Component/TickBus.h>
#include <ScriptEvents/ScriptEventsBus.h>

#include <ScriptEvents/ScriptEventSystem.h>
#include <ScriptEvents/ScriptEvent.h>

#if defined(SCRIPTEVENTS_EDITOR)

AZ_DECLARE_BUDGET(AzToolsFramework);

namespace ScriptEventsEditor
{
    ////////////////////////////
    // ScriptEventAssetHandler
    ////////////////////////////

    ScriptEventAssetHandler::ScriptEventAssetHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId, AZ::SerializeContext* serializeContext)    
        : AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>(displayName, group, extension, componentTypeId, serializeContext)
    {
    }

    AZ::Data::AssetPtr ScriptEventAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        if (type != azrtti_typeid<ScriptEvents::ScriptEventsAsset>())
        {
            return nullptr;
        }

        AZ::Data::AssetPtr assetPtr = AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>::CreateAsset(id, type);

        if (!AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::MultiHandler::BusIsConnectedId(id))
        {
            AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::MultiHandler::BusConnect(id);
        }

        return assetPtr;
    }

    void ScriptEventAssetHandler::InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload)
    {
        AssetHandler::InitAsset(asset, loadStageSucceeded, isReload);

        if (loadStageSucceeded && !isReload)
        {
            const ScriptEvents::ScriptEvent& definition = asset.GetAs<ScriptEvents::ScriptEventsAsset>()->m_definition;
            AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEventRegistration> scriptEvent;
            ScriptEvents::ScriptEventBus::BroadcastResult(scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, asset.GetId(), definition.GetVersion());
        }
    }

    AZ::Data::AssetHandler::LoadResult ScriptEventAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset, 
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        AZ::Data::AssetHandler::LoadResult loadedData = AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>::LoadAssetData(asset, stream, assetLoadFilterCB);

        if (loadedData == AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            ScriptEvents::ScriptEventsAsset* assetData = asset.GetAs<ScriptEvents::ScriptEventsAsset>();

            if (assetData)
            {
                auto busIter = m_previousEbusNames.find(asset.GetId());

                bool registerBus = true;

                if (busIter != m_previousEbusNames.end())
                {
                    if (busIter->second.m_version < assetData->m_definition.GetVersion())
                    {
                        ScriptEvents::Internal::Utils::DestroyScriptEventBehaviorEBus(busIter->second.m_previousName);
                        m_previousEbusNames.erase(busIter);
                    }
                    else
                    {
                        registerBus = false;
                    }
                }

                if (registerBus)
                {
                    // LoadAssetData is being called from an Asset system thread,
                    // we need to complete registering with the BehaviorContext in the main thread
                    auto registerBusFn = [this, assetData, asset]()
                    {
                        if (ScriptEvents::Internal::Utils::ConstructAndRegisterScriptEventBehaviorEBus(assetData->m_definition))
                        {
                            PreviousNameSettings previousSettings;
                            previousSettings.m_previousName = assetData->m_definition.GetName().c_str();
                            previousSettings.m_version = assetData->m_definition.GetVersion();

                            m_previousEbusNames[asset.GetId()] = previousSettings;                        
                        }
                    };
                    AZ::TickBus::QueueFunction(registerBusFn);

                }
            }
        }

        return loadedData;
    }

    bool ScriptEventAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        AZ_TracePrintf("ScriptEvent", "Trying to save Asset with ID: %s - SCRIPTEVENT", asset.Get()->GetId().ToString<AZStd::string>().c_str());

        // Attempt to Save the data to a temporary stream in order to see if any 
        AZ::Outcome<bool, AZStd::string> outcome = AZ::Failure(AZStd::string::format("AssetEditorValidationRequests is not connected ID: %s", asset.Get()->GetId().ToString<AZStd::string>().c_str()));

        // Verify that the asset is in a valid state that can be saved.
        AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::EventResult(outcome, asset.Get()->GetId(), &AzToolsFramework::AssetEditor::AssetEditorValidationRequests::IsAssetDataValid, asset);
        if (!outcome.IsSuccess())
        {
            AZ_Error("Asset Editor", false, "%s", outcome.GetError().c_str());
            return false;
        }

        ScriptEvents::ScriptEventsAsset* assetData = asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        AZ_Assert(assetData, "Asset is of the wrong type.");
        if (assetData && m_serializeContext)
        {
            return AZ::Utils::SaveObjectToStream<ScriptEvents::ScriptEventsAsset>(*stream,
                m_saveAsBinary ? AZ::ObjectStream::ST_BINARY : AZ::ObjectStream::ST_XML,
                assetData,
                m_serializeContext);
        }

        return false;
    }

    AZ::Outcome<bool, AZStd::string> ScriptEventAssetHandler::IsAssetDataValid(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        ScriptEvents::ScriptEventsAsset* assetData = asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        if (!assetData)
        {
            return AZ::Failure(AZStd::string::format("Unable to validate asset with id: %s it has not been registered with the Script Event system component.", asset.GetId().ToString<AZStd::string>().c_str()));
        }

        const ScriptEvents::ScriptEvent* definition = &assetData->m_definition;
        AZ_Assert(definition, "The AssetData should have a valid definition");

        return definition->Validate();
    }

    void ScriptEventAssetHandler::PreAssetSave(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        ScriptEvents::ScriptEventsAsset* scriptEventAsset = asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        scriptEventAsset->m_definition.IncreaseVersion();
    }

    void ScriptEventAssetHandler::BeforePropertyEdit(AzToolsFramework::InstanceDataNode* node, AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        ScriptEventData::VersionedProperty* property = nullptr;
        AzToolsFramework::InstanceDataNode* parent = node;
        while (parent)
        {
            if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<ScriptEventData::VersionedProperty>())
            {
                property = static_cast<ScriptEventData::VersionedProperty*>(parent->GetInstance(0));
                break;
            }
            parent = parent->GetParent();
        }

        if (property)
        {
            property->OnPropertyChange();
        }
    }

    void ScriptEventEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptEventEditorSystemComponent, AZ::Component>()
                ->Version(3)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }));
            ;
        }

        using namespace ScriptEvents;

        ScriptEventData::VersionedProperty::Reflect(context);
        Parameter::Reflect(context);
        Method::Reflect(context);
        ScriptEvent::Reflect(context);

        ScriptEventsAsset::Reflect(context);
        ScriptEventsAssetRef::Reflect(context);
        ScriptEventsAssetPtr::Reflect(context);
    }

    void ScriptEventEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptEventsService"));
    }

    void ScriptEventEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ScriptEventsService"));
    }

    ////////////////////
    // SystemComponent
    ////////////////////
    void ScriptEventEditorSystemComponent::Activate()
    {
        using namespace ScriptEvents;

        ScriptEventsSystemComponentImpl* moduleConfiguration = nullptr;
        ScriptEventModuleConfigurationRequestBus::BroadcastResult(moduleConfiguration, &ScriptEventModuleConfigurationRequests::GetSystemComponentImpl);
        if (moduleConfiguration)
        {
            moduleConfiguration->RegisterAssetHandler();
        }

        AzToolsFramework::RegisterGenericComboBoxHandler<ScriptEventData::VersionedProperty>();
    }

    void ScriptEventEditorSystemComponent::Deactivate()
    {
        using namespace ScriptEvents;
        ScriptEventsSystemComponentImpl* moduleConfiguration = nullptr;
        ScriptEventModuleConfigurationRequestBus::BroadcastResult(moduleConfiguration, &ScriptEventModuleConfigurationRequests::GetSystemComponentImpl);
        if (moduleConfiguration)
        {
            moduleConfiguration->UnregisterAssetHandler();
        }
    }
}

#endif

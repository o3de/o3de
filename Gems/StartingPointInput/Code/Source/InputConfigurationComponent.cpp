/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "InputConfigurationComponent.h"
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

namespace StartingPointInput
{
    static AZ::s32 Uint32ToInt32(const AZ::u32& value)
    {
        return static_cast<AZ::s32>(value);
    };

    InputConfigurationComponent::~InputConfigurationComponent()
    {
        m_inputEventBindings.Cleanup();
    }

    void InputConfigurationComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("InputConfigurationService"));
    }

    void InputConfigurationComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<InputConfigurationComponent, AZ::Component>()
                ->Version(4)
                ->Field("Input Event Bindings", &InputConfigurationComponent::m_inputEventBindingsAsset)
                ->Field("Local Player Index", &InputConfigurationComponent::m_localPlayerIndex)
                ->NameChange(2, 3, "Local User Id", "Local Player Index")
                ->TypeChange("Local Player Index", 3, 4, AZStd::function<AZ::s32(const AZ::u32&)>(&Uint32ToInt32))
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<InputConfigurationComponent>("Input",
                    "The Input component allows an entity to bind a set of inputs to an event by referencing a .inputbindings file")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/InputConfig.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/InputConfig.svg")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<InputEventBindingsAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/gameplay/input/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &InputConfigurationComponent::m_inputEventBindingsAsset, "Input to event bindings",
                    "Asset containing input to event binding information.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                        ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                        ->Attribute("EditButton", "")
                        ->Attribute("EditDescription", "Open in Asset Editor")
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &InputConfigurationComponent::m_localPlayerIndex, "Local player index",
                            "The player index that this component will receive input from (0 based, -1 means all controllers).\n"
                            "Will only work on platforms such as PC where the local user id corresponds to the local player index.\n"
                            "For other platforms, SetLocalUserId must be called at runtime with the id of a logged in user.")
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                        ->Attribute(AZ::Edit::Attributes::Max, 3)
                    ;
            }
        }
    }

    void InputConfigurationComponent::Init()
    {
        // The player index that this component will receive input from (0 based, -1 means all controllers)
        // can be set from data, but will only work on platforms such as PC where the local user id corresponds
        // to the local player index. For other platforms, SetLocalUserId must be called at runtime with the id
        // of a logged in user, which will overwrite anything set here from data.
        if (m_localPlayerIndex == -1)
        {
            m_localUserId = AzFramework::LocalUserIdAny;
        }
        else
        {
            // we have to cast to u32 here even if LocalUserId is not a u32 type because some platforms use
            // an aggregate type for m_localUserId and only have the pertinent constructors/operators for u32
            m_localUserId = aznumeric_cast<AZ::u32>(m_localPlayerIndex);
        }
        
    }

    void InputConfigurationComponent::Activate()
    {
        InputConfigurationComponentRequestBus::Handler::BusConnect(GetEntityId());
        AZ::Data::AssetBus::Handler::BusConnect(m_inputEventBindingsAsset.GetId());
    }

    void InputConfigurationComponent::Deactivate()
    {
        InputConfigurationComponentRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        if (m_localUserId != AzFramework::LocalUserIdNone)
        {
            m_inputEventBindings.Deactivate(m_localUserId);
        }
    }

    void InputConfigurationComponent::SetLocalUserId(AzFramework::LocalUserId localUserId)
    {
        if (m_localUserId != localUserId)
        {
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Deactivate(m_localUserId);
            }
            m_localUserId = localUserId;
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Activate(m_localUserId);
            }
        }
    }

    void InputConfigurationComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_inputEventBindingsAsset.GetId())
        {
            // before we reload and reapply, disable any existing old ones, or else they'd double up
            // and you'd end up with both being active.
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Deactivate(m_localUserId);
            }

            m_inputEventBindingsAsset = asset;
            if (asset.IsReady())
            {
                OnAssetReady(asset);
            }
        }
    }

    void InputConfigurationComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (InputEventBindingsAsset* inputAsset = asset.GetAs<InputEventBindingsAsset>())
        {
            // the input asset actually requires us to do additional cloning and copying of the data
            // mainly because we retrieve the player profile data and apply it as a bindings patch on top of the data.

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (serializeContext)
            {
                // we swap with a fresh empty one here just to make sure that if this happens repeatedly, we don't have anything left over.
                InputEventBindings freshBindings;
                serializeContext->CloneObjectInplace<InputEventBindings>(freshBindings, &inputAsset->m_bindings);
                m_inputEventBindings.Cleanup();
                m_inputEventBindings.Swap(&freshBindings);
            }

            m_isAssetPrepared = true;
            ActivateBindingsIfAppropriate();
        }
        else
        {
            AZ_Error("Input Configuration", false, "Input bindings asset is not the correct type.");
        }
    }

    void InputConfigurationComponent::ActivateBindingsIfAppropriate()
    {
        if (m_isAssetPrepared)
        {
            if (m_localUserId != AzFramework::LocalUserIdNone)
            {
                m_inputEventBindings.Activate(m_localUserId);
            }
        }
    }

    void InputConfigurationComponent::EditorSetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        m_inputEventBindingsAsset.Create(assetId);
    }
}

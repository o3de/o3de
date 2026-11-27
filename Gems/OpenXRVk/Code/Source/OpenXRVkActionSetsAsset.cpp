/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//! Breadcrumb... Because this asset serializes a field of type AZ::Data::Asset<...>
//! We need to include this file first to avoid the following compiler error:
//! error C2027: use of undefined type 'AZ::SerializeGenericTypeInfo<ValueType>'
//! error C3861: 'GetClassTypeId': identifier not found
//! The error is triggered when calling ->Field("InteractionProfilesAsset", &OpenXRActionSetsAsset::m_interactionProfilesAsset)
#include <AzCore/Asset/AssetSerializer.h>

#include <OpenXRVk/OpenXRVkActionSetsAsset.h>
#include <OpenXRVk/OpenXRVkAssetsValidator.h>

namespace OpenXRVk
{
    namespace EditorInternal
    {
        // This static asset reference is only relevant when the user is creating an OpenXRActionSetsAsset with the
        // Asset Editor. Because this variable is a singleton, creating two of these assets at the same time won't be possible.
        // But this is not an issue because most likely all OpenXRActionSetsAsset always use the same OpenXRInteractionProfilesAsset.
        static AZ::Data::Asset<OpenXRInteractionProfilesAsset> s_asset;
        static constexpr char LogName[] = "EditorInternal::OpenXRInteractionProfilesAsset";

        static void BlockingReloadAssetIfNotReady(AZ::Data::Asset<OpenXRInteractionProfilesAsset>& profileAsset)
        {
            if (!profileAsset.GetId().IsValid())
            {
                return;
            }
            if (!profileAsset.IsReady())
            {
                profileAsset.QueueLoad();
                if (profileAsset.IsLoading())
                {
                    profileAsset.BlockUntilLoadComplete();
                }
            }
        }


        static void SetCurrentInteractionProfilesAsset(AZ::Data::Asset<OpenXRInteractionProfilesAsset>& newProfilesAsset,
            bool loadAsset = true)
        {
            if (!newProfilesAsset.GetId().IsValid())
            {
                AZ_Printf(LogName, "The user cleared the global OpenXRInteractionProfilesAsset used for ActionSets Asset Editing.");
                s_asset = {};
                return;
            }
            if (loadAsset)
            {
                BlockingReloadAssetIfNotReady(newProfilesAsset);
            }
            s_asset = newProfilesAsset;
        }


        static const AZ::Data::Asset<OpenXRInteractionProfilesAsset>& GetCurrentInteractionProfilesAsset(bool loadAsset = true)
        {
            if (loadAsset)
            {
                BlockingReloadAssetIfNotReady(s_asset);
            }
            return s_asset;
        }
    }

    ///////////////////////////////////////////////////////////
    /// OpenXRActionPathDescriptor
    void OpenXRActionPathDescriptor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<OpenXRActionPathDescriptor>()
                ->Version(1)
                ->Field("InteractionProfile", &OpenXRActionPathDescriptor::m_interactionProfileName)
                ->Field("UserPath", &OpenXRActionPathDescriptor::m_userPathName)
                ->Field("ComponentPath", &OpenXRActionPathDescriptor::m_componentPathName)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<OpenXRActionPathDescriptor>("OpenXRActionPathDescriptor", "A specific OpenXR I/O action path.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &OpenXRActionPathDescriptor::GetEditorText)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &OpenXRActionPathDescriptor::m_interactionProfileName, "Interaction Profile", "The name of the Interaction Profile.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenXRActionPathDescriptor::OnInteractionProfileSelected)
                    ->Attribute(AZ::Edit::Attributes::StringList, &OpenXRActionPathDescriptor::GetInteractionProfiles)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &OpenXRActionPathDescriptor::m_userPathName, "User Path", "Name of the User Path.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenXRActionPathDescriptor::OnUserPathSelected)
                    ->Attribute(AZ::Edit::Attributes::StringList, &OpenXRActionPathDescriptor::GetUserPaths)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &OpenXRActionPathDescriptor::m_componentPathName, "I/O Component Path", "The name of I/O Component Path.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenXRActionPathDescriptor::OnComponentPathSelected)
                    ->Attribute(AZ::Edit::Attributes::StringList, &OpenXRActionPathDescriptor::GetComponentPaths)
                    ;
            }
        }
    }


    AZStd::string OpenXRActionPathDescriptor::GetEditorText() const
    {
        if (m_interactionProfileName.empty())
        {
            return "<Unknown Interaction Profile>";
        }
        if (m_userPathName.empty())
        {
            return "<Unknown User Path>";
        }
        if (m_componentPathName.empty())
        {
            return "<Unknown Component Path>";
        }
        return AZStd::string::format("%s %s %s", m_interactionProfileName.c_str(), m_userPathName.c_str(), m_componentPathName.c_str());
    }


    AZ::Crc32 OpenXRActionPathDescriptor::OnInteractionProfileSelected()
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }


    AZStd::vector<AZStd::string> OpenXRActionPathDescriptor::GetInteractionProfiles() const
    {
        const auto& interactionProfilesAsset = EditorInternal::GetCurrentInteractionProfilesAsset();
        if (!interactionProfilesAsset.IsReady())
        {
            return {};
        }
        AZStd::vector<AZStd::string> profileNames;
        for (const auto& profileDescriptor : interactionProfilesAsset->m_interactionProfileDescriptors)
        {
            profileNames.push_back(profileDescriptor.m_name);
        }
        return profileNames;
    }


    AZ::Crc32 OpenXRActionPathDescriptor::OnUserPathSelected()
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }


    AZStd::vector<AZStd::string> OpenXRActionPathDescriptor::GetUserPaths() const
    {
        const auto& interactionProfilesAsset = EditorInternal::GetCurrentInteractionProfilesAsset();
        if (!interactionProfilesAsset.IsReady())
        {
            return {};
        }

        AZStd::vector<AZStd::string> retList;
        const auto profileDescriptor = interactionProfilesAsset->GetInteractionProfileDescriptor(m_interactionProfileName);
        if (!profileDescriptor)
        {
            return retList;
        }
        for (const auto& userPathDescriptor : profileDescriptor->m_userPathDescriptors)
        {
            retList.push_back(userPathDescriptor.m_name);
        }
        return retList;
    }


    AZ::Crc32 OpenXRActionPathDescriptor::OnComponentPathSelected()
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }


    AZStd::vector<AZStd::string> OpenXRActionPathDescriptor::GetComponentPaths() const
    {
        const auto& interactionProfilesAsset = EditorInternal::GetCurrentInteractionProfilesAsset();
        if (!interactionProfilesAsset.IsReady())
        {
            return {};
        }

        AZStd::vector<AZStd::string> retList;

        const auto profileDescriptor = interactionProfilesAsset->GetInteractionProfileDescriptor(m_interactionProfileName);
        if (!profileDescriptor)
        {
            return retList;
        }

        const auto* userPathDescriptor = profileDescriptor->GetUserPathDescriptor(m_userPathName);
        if (!userPathDescriptor)
        {
            return retList;
        }

        for (const auto& componentPath : userPathDescriptor->m_componentPathDescriptors)
        {
            retList.push_back(componentPath.m_name);
        }

        for (const auto& componentPath : profileDescriptor->m_commonComponentPathDescriptors)
        {
            retList.push_back(componentPath.m_name);
        }

        return retList;
    }
    /// OpenXRActionPathDescriptor
    ///////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////
    /// OpenXRActionDescriptor
    void OpenXRActionDescriptor::Reflect(AZ::ReflectContext* context)
    {
        OpenXRActionPathDescriptor::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<OpenXRActionDescriptor>()
                ->Version(1)
                ->Field("Name", &OpenXRActionDescriptor::m_name)
                ->Field("LocalizedName", &OpenXRActionDescriptor::m_localizedName)
                ->Field("Comment", &OpenXRActionDescriptor::m_comment)
                ->Field("ActionPathDescriptors", &OpenXRActionDescriptor::m_actionPathDescriptors)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<OpenXRActionDescriptor>("OpenXRActionDescriptor", "An action bound to one or more OpenXR Action Paths.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &OpenXRActionDescriptor::GetEditorText)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionDescriptor::m_name, "Name", "Runtime identifier for this action.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionDescriptor::m_localizedName, "Localized Name", "User friendly display name.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues"))
                    ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &OpenXRActionDescriptor::m_comment, "Comment", "Free form description of this Action Set.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionDescriptor::m_actionPathDescriptors, "Action Paths", "List of action paths bound to this action")
                    ;
            }
        }
    }


    AZStd::string OpenXRActionDescriptor::GetEditorText() const
    {
        // In addition to showing the Action name, we'll append the Action Data Type
        // of the first ActionPath, so it is easy for the developer to see what will be
        // the expected data type at runtime.
        AZStd::string actionTypeStr = "Unknown Type";
        if (!m_actionPathDescriptors.empty())
        {
            if (EditorInternal::s_asset)
            {
                actionTypeStr = EditorInternal::s_asset->GetActionPathTypeStr(
                    m_actionPathDescriptors[0].m_interactionProfileName,
                    m_actionPathDescriptors[0].m_userPathName,
                    m_actionPathDescriptors[0].m_componentPathName
                );
            }
        }
        AZStd::string actionDescription = AZStd::string::format("%s [%s]", m_name.empty() ? "<Unknown Action>" : m_name.c_str(), actionTypeStr.c_str());
        return actionDescription;
    }
    /// OpenXRActionDescriptor
    ///////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////
    /// OpenXRActionSetDescriptor
    void OpenXRActionSetDescriptor::Reflect(AZ::ReflectContext* context)
    {
        OpenXRActionDescriptor::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<OpenXRActionSetDescriptor>()
                ->Version(1)
                ->Field("Name", &OpenXRActionSetDescriptor::m_name)
                ->Field("LocalizedName", &OpenXRActionSetDescriptor::m_localizedName)
                ->Field("Priority", &OpenXRActionSetDescriptor::m_priority)
                ->Field("Comment", &OpenXRActionSetDescriptor::m_comment)
                ->Field("ActionDescriptors", &OpenXRActionSetDescriptor::m_actionDescriptors)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<OpenXRActionSetDescriptor>("OpenXRActionSetDescriptor", "Contains a list of OpenXR Actions.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &OpenXRActionSetDescriptor::GetEditorText)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionSetDescriptor::m_name, "Name", "Runtime identifier for this action set.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionSetDescriptor::m_localizedName, "Localized Name", "Action set display name.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues"))
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &OpenXRActionSetDescriptor::m_priority, "Priority", "The higher this value the higher the priority.")
                    ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &OpenXRActionSetDescriptor::m_comment, "Comment", "Free form description of this Action Set.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionSetDescriptor::m_actionDescriptors, "Actions", "List of actions for this action set.")
                    ;
            }
        }
    }


    AZStd::string OpenXRActionSetDescriptor::GetEditorText() const
    {
        if (!m_name.empty())
        {
            return m_name;
        }
        return m_localizedName.empty() ? "<Unknown Action Set>" : m_localizedName;
    }
    /// OpenXRActionSetDescriptor
    ///////////////////////////////////////////////////////////
    

    ///////////////////////////////////////////////////////////
    /// OpenXRActionBindingsAsset
    void OpenXRActionSetsAsset::Reflect(AZ::ReflectContext* context)
    {
        OpenXRActionSetDescriptor::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<OpenXRActionSetsAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("InteractionProfilesAsset", &OpenXRActionSetsAsset::m_interactionProfilesAsset)
                ->Field("ActionSetDescriptors", &OpenXRActionSetsAsset::m_actionSetDescriptors)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<OpenXRActionSetsAsset>(
                    s_assetTypeName, "Defines the OpenXR Actions an application cares about.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionSetsAsset::m_interactionProfilesAsset, "Interaction Profiles Asset", "This asset determines the hardware systems that are compatible with these Action Sets.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenXRActionSetsAsset::OnInteractionProfilesAssetChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &OpenXRActionSetsAsset::m_actionSetDescriptors, "Action Sets", "List of action sets.")
                    ;
            }
        }
    }


    AZ::Crc32 OpenXRActionSetsAsset::OnInteractionProfilesAssetChanged()
    {
        EditorInternal::SetCurrentInteractionProfilesAsset(m_interactionProfilesAsset);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
    /// OpenXRActionBindingsAsset
    ///////////////////////////////////////////////////////////
    

    ///////////////////////////////////////////////////////////
    /// OpenXRActionBindingsAssetHandler
    OpenXRActionSetsAssetHandler::OpenXRActionSetsAssetHandler()
        : AzFramework::GenericAssetHandler<OpenXRActionSetsAsset>(
            OpenXRActionSetsAsset::s_assetTypeName,
            "Other",
            OpenXRActionSetsAsset::s_assetExtension)
    {
    }
    

    AZ::Data::AssetHandler::LoadResult OpenXRActionSetsAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        auto actionSetsAsset = asset.GetAs<OpenXRActionSetsAsset>();
        if (!actionSetsAsset)
        {
            AZ_Error("OpenXRActionSetsAssetHandler", false, "This should be a OpenXRActionSetsAsset, as this is the only type we process.");
            return AZ::Data::AssetHandler::LoadResult::Error;
        }
    
        if (!AZ::Utils::LoadObjectFromStreamInPlace(
            *stream, *actionSetsAsset, nullptr, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB)))
        {
            return AZ::Data::AssetHandler::LoadResult::Error;
        }

        // The reason we don't load the interaction profiles asset upon construction
        // is because we only want to load the singleton asset only if we are 100% sure
        // this asset is being edited by the Asset Editor.
        // Remember that at game runtime, there's no such thing as the Asset Editor.
        constexpr bool loadAsset = false;
        EditorInternal::SetCurrentInteractionProfilesAsset(actionSetsAsset->m_interactionProfilesAsset, loadAsset);
    
        return AZ::Data::AssetHandler::LoadResult::LoadComplete;
    
    }


    bool OpenXRActionSetsAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        OpenXRActionSetsAsset* assetData = asset.GetAs<OpenXRActionSetsAsset>();
        if (!assetData->m_interactionProfilesAsset.GetId().IsValid())
        {
            AZ_Error("OpenXRActionSetsAssetHandler", false, "Can't save this OpenXRActionSetsAsset without a valid OpenXRInteractionProfilesAsset")
            return false;
        }

        if (!assetData->m_interactionProfilesAsset.IsReady())
        {
            EditorInternal::SetCurrentInteractionProfilesAsset(assetData->m_interactionProfilesAsset);
        }

        auto outcome = OpenXRVkAssetsValidator::ValidateActionSetsAsset(*assetData, *(assetData->m_interactionProfilesAsset.Get()));
        if (!outcome.IsSuccess())
        {
            AZ_Error("OpenXRActionSetsAssetHandler", false, "Can't save this OpenXRActionSetsAsset. Reason:\n%s", outcome.GetError().c_str());
            return false;
        }

        return AzFramework::GenericAssetHandler<OpenXRActionSetsAsset>::SaveAssetData(asset, stream);
    }
    /// OpenXRActionBindingsAssetHandler
    ///////////////////////////////////////////////////////////

} // namespace OpenXRVk

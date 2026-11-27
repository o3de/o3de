/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TickBus.h>
#include <AzCore/Settings/SettingsRegistry.h>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

//#include <OpenXRVk/OpenXRInteractionProfileBus.h>
#include <OpenXRVk/OpenXRVkUtils.h>
#include <OpenXRVk/OpenXRVkReferenceSpacesInterface.h>
#include "OpenXRVkActionsManager.h"

namespace OpenXRVk
{
    // The action type of an action will always be the action type of the first action path descriptor. 
    static XrActionType GetActionTypeFromActionDescriptor(const OpenXRInteractionProfilesAsset& interactionProfilesAsset,
        const OpenXRActionDescriptor& actionDescriptor)
    {
        if (actionDescriptor.m_actionPathDescriptors.empty())
        {
            AZ_Error(ActionsManager::LogName, false, "Need at least one action path descriptor for action with name [%s].", actionDescriptor.m_name.c_str());
            return XR_ACTION_TYPE_MAX_ENUM;
        }

        const auto& actionPathDescriptor = actionDescriptor.m_actionPathDescriptors[0];

        auto interactionProfileDescriptor = interactionProfilesAsset.GetInteractionProfileDescriptor(actionPathDescriptor.m_interactionProfileName);
        if (!interactionProfileDescriptor)
        {
            AZ_Error(ActionsManager::LogName, false, "Couldn't find interaction profile named [%s].", actionPathDescriptor.m_interactionProfileName.c_str());
            return XR_ACTION_TYPE_MAX_ENUM;
        }

        auto userPathDescriptor = interactionProfileDescriptor->GetUserPathDescriptor(actionPathDescriptor.m_userPathName);
        if (!userPathDescriptor)
        {
            AZ_Error(ActionsManager::LogName, false, "Couldn't find user path descriptor with name [%s].", actionPathDescriptor.m_userPathName.c_str());
            return XR_ACTION_TYPE_MAX_ENUM;
        }

        // See if the component is owned by the user path.
        auto componentPathDescriptor = userPathDescriptor->GetComponentPathDescriptor(actionPathDescriptor.m_componentPathName);
        if (!componentPathDescriptor)
        {
            componentPathDescriptor = interactionProfileDescriptor->GetCommonComponentPathDescriptor(actionPathDescriptor.m_componentPathName);
            if (!componentPathDescriptor)
            {
                AZ_Error(ActionsManager::LogName, false, "Couldn't find component path descriptor for component path with name [%s].", actionPathDescriptor.m_componentPathName.c_str());
                return XR_ACTION_TYPE_MAX_ENUM;
            }
        }

        return componentPathDescriptor->GetXrActionType();
    }


    AZStd::string ActionsManager::GetActionSetsAssetPath()
    {
        // Asset Cache relative path
        static constexpr char DefaultActionsAssetPath[] = "openxrvk/default.xractions";
        static constexpr char ActionSetsAssetPathRegistryKey[] = "/O3DE/Atom/OpenXR/ActionSetsAsset";

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            AZStd::string customPath;
            bool keyExists = settingsRegistry->Get(customPath, ActionSetsAssetPathRegistryKey);
            if (keyExists && !customPath.empty())
            {
                AZ_Printf(LogName, "The application defined a custom ActionSets asset at path [%s].\n",
                    customPath.c_str());
                return customPath;
            }
            
        }
        AZ_Printf(LogName, "The application will use the default ActionSets asset at path [%s].\n",
            DefaultActionsAssetPath);
        return AZStd::string(DefaultActionsAssetPath);
    }


    bool ActionsManager::LoadActionSetAssetAsync()
    {
        auto productPath = GetActionSetsAssetPath();
        auto assetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(productPath.c_str(), AZ::RPI::AssetUtils::TraceLevel::Error);
        if (!assetId.IsValid())
        {
            AZ_Error(LogName, false, "No ActionSet Asset named [%s] was found in the asset catalog.", productPath.c_str());
            return true; // We return true, just in case we are running on the Editor and the user needs the Editor to verify the content of the asset.
        }

        AZ::Data::AssetBus::Handler::BusConnect(assetId);

        m_actionSetAsset = AZ::Data::AssetManager::Instance().GetAsset(assetId,
            azrtti_typeid<OpenXRActionSetsAsset>(), AZ::Data::AssetLoadBehavior::PreLoad);
        m_actionSetAsset.QueueLoad();
        return true;
    }


    ActionsManager::~ActionsManager()
    {
        if (AZ::Data::AssetBus::Handler::BusIsConnected())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }
    }


    bool ActionsManager::Init(XrInstance xrInstance, XrSession xrSession)
    {
        m_xrInstance = xrInstance;
        m_xrSession = xrSession;

        auto outcome = SetBaseReferenceSpaceForPoseActions(IOpenXRReferenceSpaces::ReferenceSpaceNameView);
        if (!outcome.IsSuccess())
        {
            const auto outcomeMsg = outcome.TakeError();
            auto errorMsg = AZStd::string::format("Failed to set [%s] as the default base visualized space. Reason:\n%s.",
                IOpenXRReferenceSpaces::ReferenceSpaceNameView, outcomeMsg.c_str());
            AZ_Assert(false, "%s", errorMsg.c_str());
            AZ_Error(LogName, false, "%s", errorMsg.c_str());
            return false;
        }

        return LoadActionSetAssetAsync();
    }


    void ActionsManager::InitInternal()
    {
        AZ_Assert(m_actionSetAsset.IsReady(), "Action Sets asset should be ready!");
        AZ_Assert(m_actionSetAsset->m_interactionProfilesAsset.IsReady(), "Was expecting that the Asset System would have loaded the InteractionProfiles Asset.");

        const auto interactionProfilesAsset = m_actionSetAsset->m_interactionProfilesAsset;

        SuggestedBindingsPerProfile suggestedBindingsPerProfile;
        for (const auto& actionSetDescriptor : m_actionSetAsset->m_actionSetDescriptors)
        {
            if (!InitActionSet(*interactionProfilesAsset, actionSetDescriptor, suggestedBindingsPerProfile))
            {
                return;
            }
        }

        if (suggestedBindingsPerProfile.empty())
        {
            AZ_Printf(LogName, "This application will run without actions.\n");
            return;
        }

        // Register the bindings for each active interaction profile.
        uint32_t activeProfileCount = 0;
        for (const auto& [profileName, suggestedBindings] : suggestedBindingsPerProfile)
        {
            XrInteractionProfileSuggestedBinding xrSuggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
            xrSuggestedBindings.interactionProfile = suggestedBindings.m_profileXrPath;
            xrSuggestedBindings.suggestedBindings = suggestedBindings.m_suggestedBindingsList.data();
            xrSuggestedBindings.countSuggestedBindings = static_cast<uint32_t>(suggestedBindings.m_suggestedBindingsList.size());
            XrResult result = xrSuggestInteractionProfileBindings(m_xrInstance, &xrSuggestedBindings);
            if (IsError(result))
            {
                PrintXrError(LogName, result, "Got an error during suggested bindings registration for profile [%s].", profileName.c_str());
            }
            else
            {
                AZ_Printf(LogName, "Successfully registred action bindings for profile [%s].\n", profileName.c_str());
                activeProfileCount++;
            }
        }
        if (activeProfileCount < 1)
        {
            m_topLevelUserPaths.clear();
            AZ_Error(LogName, false, "Failed to activate at least one interaction profile. This application will run without actions.\n");
            return;
        }

        AZStd::vector<XrActionSet> xrActionSets;
        xrActionSets.reserve(m_actionSets.size());
        size_t actionSetIdx = 0;
        for (const auto& actionSetInfo : m_actionSets)
        {
            xrActionSets.push_back(actionSetInfo.m_xrActionSet);
            m_activeActionSets.set(actionSetIdx, true); // By default all actionSets will be active.
            actionSetIdx++;
        }
        RecreateXrActiveActionSets();

        XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
        attachInfo.countActionSets = static_cast<uint32_t>(xrActionSets.size());
        attachInfo.actionSets = xrActionSets.data();
        XrResult result = xrAttachSessionActionSets(m_xrSession, &attachInfo);
        if (IsError(result))
        {
            m_xrActiveActionSets.clear();
            PrintXrError(LogName, result, "Failed to attach %zu action sets to the session.", xrActionSets.size());
        }

        AZ_Printf(LogName, "Successfully configure the Action Sets.");
    }


    bool ActionsManager::SyncActions(XrTime predictedDisplayTime)
    {
        if (m_xrActiveActionSets.empty())
        {
            // Nothing to do
            return true;
        }

        m_predictedDisplaytime = predictedDisplayTime;

        XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
        syncInfo.countActiveActionSets = aznumeric_cast<uint32_t>(m_xrActiveActionSets.size());
        syncInfo.activeActionSets = m_xrActiveActionSets.data();
        XrResult result = xrSyncActions(m_xrSession, &syncInfo);
        if (IsError(result))
        {
            PrintXrError(LogName, result, "Failed to sync %zu actionSets.\n", m_xrActiveActionSets.size());
            return false;
        }

        return true;
    }


    bool ActionsManager::InitActionSet(const OpenXRInteractionProfilesAsset& interactionProfilesAsset,
        const OpenXRActionSetDescriptor& actionSetDescriptor,
        SuggestedBindingsPerProfile& suggestedBindingsPerProfile)
    {
        // Create an action set.
        XrActionSetCreateInfo actionSetCreateInfo{};
        actionSetCreateInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
        azstrcpy(actionSetCreateInfo.actionSetName, sizeof(actionSetCreateInfo.actionSetName), actionSetDescriptor.m_name.c_str());
        const char* localizedNameCStr = actionSetDescriptor.m_name.c_str();
        if (!actionSetDescriptor.m_localizedName.empty())
        {
            localizedNameCStr = actionSetDescriptor.m_localizedName.c_str();
        }
        azstrcpy(actionSetCreateInfo.localizedActionSetName, sizeof(actionSetCreateInfo.localizedActionSetName), localizedNameCStr);
        actionSetCreateInfo.priority = actionSetDescriptor.m_priority;

        {
            ActionSetInfo newActionSetInfo;
            newActionSetInfo.m_name = actionSetDescriptor.m_name;
            XrResult result = xrCreateActionSet(m_xrInstance, &actionSetCreateInfo, &newActionSetInfo.m_xrActionSet);
            if (IsError(result))
            {
                PrintXrError(LogName, result, "Failed to instantiate actionSet named [%s].", actionSetDescriptor.m_name.c_str());
                return false;
            }
            m_actionSets.emplace_back(AZStd::move(newActionSetInfo));
        }

        ActionSetInfo& newActionSetInfo = m_actionSets.back();
        for (const auto& actionDescriptor : actionSetDescriptor.m_actionDescriptors)
        {
            if (!AddActionToActionSet(interactionProfilesAsset, newActionSetInfo, actionDescriptor, suggestedBindingsPerProfile))
            {
                AZ_Error(LogName, false, "Failed to initialize action bindings for action named [%s] under actionSet named [%s].",
                    actionDescriptor.m_name.c_str(), actionSetDescriptor.m_name.c_str());
                return false;
            }
        }
        
        return true;
    }


    bool ActionsManager::AddActionToActionSet(const OpenXRInteractionProfilesAsset& interactionProfilesAsset,
        ActionSetInfo& actionSetInfo,
        const OpenXRActionDescriptor& actionDescriptor,
        SuggestedBindingsPerProfile& suggestedBindingsPerProfile)
    {
        // One OpenXRAction object will become one XrAction.
        // An OpenXRAction contains a list of OpenXRActionPath that need to be bound.
        // The action type for each XrAction will be the same and it will be determined by
        // the action type of the first action in the list. 
        AZ_Assert(!actionDescriptor.m_actionPathDescriptors.empty(), "An action descriptor must contain at least one action path descriptor.");
        XrActionType actionType = GetActionTypeFromActionDescriptor(interactionProfilesAsset, actionDescriptor);

        XrSpace newXrActionSpace = XR_NULL_HANDLE; // Optional.
        XrAction newXrAction = CreateXrActionAndXrSpace(actionSetInfo, actionDescriptor, actionType, newXrActionSpace);
        if (newXrAction == XR_NULL_HANDLE)
        {
            return false;
        }

        // For each actionPath in the list, create the XrPath and its binding.
        const auto additionalBindingsCount = AppendActionBindings(interactionProfilesAsset, actionDescriptor, newXrAction, suggestedBindingsPerProfile);
        if (additionalBindingsCount < 1)
        {
            // This action has no bindings. Don't add it to the active actions list.
            AZ_Warning(LogName, false, "The action [%s] had no bindings!.\n", actionDescriptor.m_name.c_str());
            if (newXrActionSpace != XR_NULL_HANDLE)
            {
                xrDestroySpace(newXrActionSpace);
            }
            xrDestroyAction(newXrAction);
            return true;
        }

        m_actions.push_back({});
        auto& newActionInfo = m_actions.back();
        newActionInfo.m_name = actionDescriptor.m_name;
        newActionInfo.m_actionType = actionType;
        newActionInfo.m_xrAction = newXrAction;
        newActionInfo.m_xrSpace = newXrActionSpace;

        uint16_t newActionIndex = aznumeric_cast<uint16_t>(m_actions.size() - 1);
        actionSetInfo.m_actions.emplace(actionDescriptor.m_name, IOpenXRActions::ActionHandle(newActionIndex));

        return true;
    }


    XrAction ActionsManager::CreateXrActionAndXrSpace(const ActionSetInfo& actionSetInfo,
        const OpenXRActionDescriptor& actionDescriptor, const XrActionType actionType, XrSpace& newXrActionSpace) const
    {
        XrActionCreateInfo actionCreateInfo{ XR_TYPE_ACTION_CREATE_INFO };
        actionCreateInfo.actionType = actionType;
        azstrcpy(actionCreateInfo.actionName, sizeof(actionCreateInfo.actionName), actionDescriptor.m_name.c_str());
        const char* localizedNameCStr = actionDescriptor.m_name.c_str();
        if (!actionDescriptor.m_localizedName.empty())
        {
            localizedNameCStr = actionDescriptor.m_localizedName.c_str();
        }
        azstrcpy(actionCreateInfo.localizedActionName, sizeof(actionCreateInfo.localizedActionName), localizedNameCStr);
        actionCreateInfo.countSubactionPaths = 0; // Subactions are not supported.
        actionCreateInfo.subactionPaths = nullptr; // Subactions are not supported.

        XrAction newXrAction;
        XrResult result = xrCreateAction(actionSetInfo.m_xrActionSet, &actionCreateInfo, &newXrAction);
        if (IsError(result))
        {
            PrintXrError(LogName, result, "Failed to create XrAction named %s.\n", actionDescriptor.m_name.c_str());
            return XR_NULL_HANDLE;
        }

        // The space will be relevant if the action type is POSE. 
        newXrActionSpace = XR_NULL_HANDLE;
        if (actionType == XR_ACTION_TYPE_POSE_INPUT)
        {
            XrActionSpaceCreateInfo actionSpaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
            actionSpaceInfo.action = newXrAction;
            actionSpaceInfo.poseInActionSpace.orientation.w = 1.f; // Make it an identity quaterion.
            result = xrCreateActionSpace(m_xrSession, &actionSpaceInfo, &newXrActionSpace);
            if (IsError(result))
            {
                xrDestroyAction(newXrAction);
                PrintXrError(LogName, result, "Failed to create XrSpace for action named %s.\n", actionDescriptor.m_name.c_str());
                return XR_NULL_HANDLE;
            }
        }

        return newXrAction;
    }


    uint32_t ActionsManager::AppendActionBindings(const OpenXRInteractionProfilesAsset& interactionProfilesAsset,
        const OpenXRActionDescriptor& actionDescriptor, XrAction xrAction,
        SuggestedBindingsPerProfile& suggestedBindingsPerProfile) const
    {
        uint32_t additionalBindingsCount = 0;
        for (const auto& actionPathDescriptor : actionDescriptor.m_actionPathDescriptors)
        {
            auto interactionProfileDescriptor = interactionProfilesAsset.GetInteractionProfileDescriptor(actionPathDescriptor.m_interactionProfileName);
            if (!interactionProfileDescriptor)
            {
                AZ_Error(LogName, false, "Couldn't find interaction profile descriptor with name [%s].", actionPathDescriptor.m_interactionProfileName.c_str());
                return false;
            }

            auto userPathDescriptor = interactionProfileDescriptor->GetUserPathDescriptor(actionPathDescriptor.m_userPathName);
            if (!userPathDescriptor)
            {
                AZ_Error(LogName, false, "Couldn't find user path descriptor with name [%s].", actionPathDescriptor.m_userPathName.c_str());
                return false;
            }

            m_topLevelUserPaths.emplace(userPathDescriptor->m_path);

            const auto absoluteComponentPath = interactionProfileDescriptor->GetComponentAbsolutePath(*userPathDescriptor, actionPathDescriptor.m_componentPathName);
            if (absoluteComponentPath.empty())
            {
                AZ_Error(LogName, false, "Failed to retrieve the absolute action path for profile [%s], user path [%s], component path [%s].\n",
                    actionPathDescriptor.m_interactionProfileName.c_str(), actionPathDescriptor.m_userPathName.c_str(), actionPathDescriptor.m_componentPathName.c_str());
                return false;
            }

            XrPath xrBindingPath;
            XrResult result = xrStringToPath(m_xrInstance, absoluteComponentPath.c_str(), &xrBindingPath);
            if (IsError(result))
            {
                PrintXrError(LogName, result, "Failed to create XrPath for action with profile [%s], absolute path [%s].\n",
                    actionPathDescriptor.m_interactionProfileName.c_str(), absoluteComponentPath.c_str());
                continue;
            }

            // If the interaction profile is not in the dictionary, then add it.
            if (!suggestedBindingsPerProfile.contains(interactionProfileDescriptor->m_name))
            {
                XrPath profileXrPath;
                result = xrStringToPath(m_xrInstance, interactionProfileDescriptor->m_path.c_str(), &profileXrPath);
                if (IsError(result))
                {
                    PrintXrError(LogName, result, "Failed to get profile [%s] XrPath while working on action [%s]",
                        interactionProfileDescriptor->m_path.c_str(), actionDescriptor.m_name.c_str());
                    continue;
                }
                suggestedBindingsPerProfile.emplace(interactionProfileDescriptor->m_name, SuggestedBindings{});
                SuggestedBindings& newProfileBindings = suggestedBindingsPerProfile.at(interactionProfileDescriptor->m_name);
                newProfileBindings.m_profileXrPath = profileXrPath;
            }
            SuggestedBindings& profileBindings = suggestedBindingsPerProfile.at(interactionProfileDescriptor->m_name);

            XrActionSuggestedBinding binding;
            binding.action = xrAction;
            binding.binding = xrBindingPath;
            profileBindings.m_suggestedBindingsList.push_back(binding);
            additionalBindingsCount++;
        }

        return additionalBindingsCount;
    }


    void ActionsManager::OnInteractionProfileChanged()
    {
        if (m_topLevelUserPaths.empty())
        {
            return;
        }

        uint32_t activeUserPathsCount = 0;
        for (const auto& userPathStr : m_topLevelUserPaths)
        {
            XrPath xrUserPath;
            XrResult result = xrStringToPath(m_xrInstance, userPathStr.c_str(), &xrUserPath);
            if (IsError(result))
            {
                PrintXrError(LogName, result, "Failed to get xrPath for user top path [%s]", userPathStr.c_str());
                continue;
            }
            XrInteractionProfileState profileStateOut{ XR_TYPE_INTERACTION_PROFILE_STATE };
            result = xrGetCurrentInteractionProfile(
                m_xrSession, xrUserPath, &profileStateOut);
            if (IsError(result))
            {
                PrintXrError(LogName, result, "Failed to get active profile for user top path [%s]", userPathStr.c_str());
                continue;
            }
            if (profileStateOut.interactionProfile == XR_NULL_PATH)
            {
                AZ_Printf(LogName, "Got an NULL Interaction Profile for [%s].\n", userPathStr.c_str());
                continue;
            }
            AZStd::string activeProfileName = ConvertXrPathToString(m_xrInstance, profileStateOut.interactionProfile);
            if (activeProfileName.empty())
            {
                AZ_Error(LogName, false, "Failed to convert Interaction Profile XrPath to string for user top path [%s]", userPathStr.c_str());
                continue;
            }
            else
            {
                activeUserPathsCount++;
                AZ_Printf(LogName, "Current Interaction Profile for [%s] is [%s].\n", userPathStr.c_str(), activeProfileName.c_str());
            }
        }
        if (!activeUserPathsCount)
        {
            AZ_Printf(LogName, "No interaction profile is active at the moment.\n");
        }
    }


    /////////////////////////////////////////////////
    /// OpenXRActionsInterface overrides
    AZStd::vector<AZStd::string> ActionsManager::GetAllActionSets() const
    {
        AZStd::vector<AZStd::string> retList;
        retList.reserve(m_actionSets.size());
        for (const auto& actionSetInfo : m_actionSets)
        {
            retList.push_back(actionSetInfo.m_name);
        }
        return retList;
    }


    AZ::Outcome<bool, AZStd::string> ActionsManager::GetActionSetState(const AZStd::string& actionSetName) const
    {
        for (size_t i = 0; i < m_actionSets.size(); i++)
        {
            if (m_actionSets[i].m_name == actionSetName)
            {
                return AZ::Success(m_activeActionSets[i]);
            }
        }
        return AZ::Failure(
            AZStd::string::format("ActionSet with name [%s] not found.", actionSetName.c_str())
        );
    }


    AZ::Outcome<bool, AZStd::string> ActionsManager::SetActionSetState(const AZStd::string& actionSetName, bool activate)
    {
        for (size_t i = 0; i < m_actionSets.size(); i++)
        {
            if (m_actionSets[i].m_name == actionSetName)
            {
                bool prevState = m_activeActionSets[i];
                if (prevState != activate)
                {
                    m_activeActionSets[i] = activate;
                    RecreateXrActiveActionSets();
                }
                return AZ::Success(prevState);
            }
        }
        return AZ::Failure(
            AZStd::string::format("ActionSet with name [%s] not found.", actionSetName.c_str())
        );
    }


    IOpenXRActions::ActionHandle ActionsManager::GetActionHandle(const AZStd::string& actionSetName, const AZStd::string& actionName) const
    {
        for (const auto& actionSetInfo : m_actionSets)
        {
            if (actionSetInfo.m_name != actionSetName)
            {
                continue;
            }
            const auto itor = actionSetInfo.m_actions.find(actionName);
            if (itor == actionSetInfo.m_actions.end())
            {
                AZ_Error(LogName, false, "ActionSet with name [%s] does not have an action named [%s].",
                    actionSetName.c_str(), actionName.c_str());
                return IOpenXRActions::ActionHandle{};
            }
            return AZ::Success(itor->second);
        }
        AZ_Error(LogName, false, "ActionSet with name [%s] not found.", actionSetName.c_str());
        return IOpenXRActions::ActionHandle{};
    }


    AZ::Outcome<bool, AZStd::string> ActionsManager::GetActionStateBoolean(ActionHandle actionHandle) const
    {
        if (!actionHandle.IsValid())
        {
            return AZ::Failure("Invalid actionHandle!");
        }
        const auto actionIndex = actionHandle.GetIndex();

        XrActionStateBoolean state { XR_TYPE_ACTION_STATE_BOOLEAN };
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = m_actions[actionIndex].m_xrAction;
        XrResult result = xrGetActionStateBoolean(m_xrSession, &getInfo, &state);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }
        if (!state.isActive)
        {
            return AZ::Failure(
                AZStd::string::format("Boolean Action [%s] is NOT active.", m_actions[actionIndex].m_name.c_str())
            );
        }

        return AZ::Success(state.currentState);
    }


    AZ::Outcome<float, AZStd::string> ActionsManager::GetActionStateFloat(ActionHandle actionHandle) const
    {
        if (!actionHandle.IsValid())
        {
            return AZ::Failure("Invalid actionHandle!");
        }
        const auto actionIndex = actionHandle.GetIndex();

        XrActionStateFloat state{ XR_TYPE_ACTION_STATE_FLOAT };
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = m_actions[actionIndex].m_xrAction;
        XrResult result = xrGetActionStateFloat(m_xrSession, &getInfo, &state);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }
        if (!state.isActive)
        {
            return AZ::Failure(
                AZStd::string::format("Float Action [%s] is NOT active.", m_actions[actionIndex].m_name.c_str())
            );
        }

        return AZ::Success(state.currentState);
    }


    AZ::Outcome<AZ::Vector2, AZStd::string> ActionsManager::GetActionStateVector2(ActionHandle actionHandle) const
    {
        if (!actionHandle.IsValid())
        {
            return AZ::Failure("Invalid actionHandle!");
        }
        const auto actionIndex = actionHandle.GetIndex();

        XrActionStateVector2f state{ XR_TYPE_ACTION_STATE_VECTOR2F };
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = m_actions[actionIndex].m_xrAction;
        XrResult result = xrGetActionStateVector2f(m_xrSession, &getInfo, &state);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }
        if (!state.isActive)
        {
            return AZ::Failure(
                AZStd::string::format("Vector2 Action[%s] is NOT active.", m_actions[actionIndex].m_name.c_str())
            );
        }

        return AZ::Success(AZ::Vector2(state.currentState.x, state.currentState.y));
    }


    AZ::Outcome<bool, AZStd::string> ActionsManager::SetBaseReferenceSpaceForPoseActions(const AZStd::string& visualizedSpaceName)
    {
        if (visualizedSpaceName == m_baseReferenceSpaceName)
        {
            return AZ::Success(true);
        }

        auto visualizedSpacesIface = OpenXRReferenceSpacesInterface::Get();
        if (!visualizedSpacesIface)
        {
            AZ_Assert(false, "The OpenXRReferenceSpacesInterface doesn't exist!");
            return AZ::Failure("The OpenXRReferenceSpacesInterface doesn't exist!");
        }

        const void* opaqueXrSpace = visualizedSpacesIface->GetReferenceSpaceNativeHandle(visualizedSpaceName);
        if (!opaqueXrSpace)
        {
            return AZ::Failure(
                AZStd::string::format("Visualized space with name [%s] doesn't exist. Will keep the current base space named [%s]",
                                      visualizedSpaceName.c_str(), m_baseReferenceSpaceName.c_str())
            );
        }
        m_baseReferenceSpaceName = visualizedSpaceName;
        m_xrBaseReferenceSpace = reinterpret_cast<XrSpace>(const_cast<void*>(opaqueXrSpace));
        return AZ::Success(true);
    }


    const AZStd::string& ActionsManager::GetBaseReferenceSpaceForPoseActions() const
    {
        return m_baseReferenceSpaceName;
    }


    AZ::Outcome<AZ::Transform, AZStd::string> ActionsManager::GetActionStatePose(ActionHandle actionHandle) const
    {
        if (!actionHandle.IsValid())
        {
            return AZ::Failure("Invalid actionHandle!");
        }
        const auto actionIndex = actionHandle.GetIndex();

        // First, we need to make sure the Action is active.
        XrActionStatePose state{ XR_TYPE_ACTION_STATE_POSE };
        XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = m_actions[actionIndex].m_xrAction;
        XrResult result = xrGetActionStatePose(m_xrSession, &getInfo, &state);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }
        if (!state.isActive)
        {
            return AZ::Failure(
                AZStd::string::format("Pose Action [%s] is NOT active.", m_actions[actionIndex].m_name.c_str())
            );
        }

        XrSpaceLocation spaceLocation {XR_TYPE_SPACE_LOCATION};
        result = xrLocateSpace(m_actions[actionIndex].m_xrSpace, m_xrBaseReferenceSpace, m_predictedDisplaytime, &spaceLocation);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }

        AZ::Vector3 poseLocation = AZ::Vector3::CreateZero();
        if (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
        {
            poseLocation = AzPositionFromXrPose(spaceLocation.pose);
        }
        
        AZ::Quaternion poseOrientation = AZ::Quaternion::CreateIdentity();
        if (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
        {
            poseOrientation = AzQuaternionFromXrPose(spaceLocation.pose);
        }

        AZ::Transform retPoseTransform = AZ::Transform::CreateFromQuaternionAndTranslation(poseOrientation, poseLocation);
        return AZ::Success(retPoseTransform);
    }


    AZ::Outcome<PoseWithVelocities, AZStd::string> ActionsManager::GetActionStatePoseWithVelocities(ActionHandle actionHandle) const
    {
        if (!actionHandle.IsValid())
        {
            return AZ::Failure("Invalid actionHandle!");
        }
        const auto actionIndex = actionHandle.GetIndex();

        XrSpaceVelocity spaceVelocity{ XR_TYPE_SPACE_VELOCITY };
        XrSpaceLocation spaceLocation{ XR_TYPE_SPACE_LOCATION, &spaceVelocity};
        XrResult result = xrLocateSpace(m_actions[actionIndex].m_xrSpace, m_xrBaseReferenceSpace, m_predictedDisplaytime, &spaceLocation);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }

        AZ::Vector3 poseLocation = AZ::Vector3::CreateZero();
        AZ::Vector3 linearVelocity = AZ::Vector3::CreateZero();
        if (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
        {
            poseLocation = AzPositionFromXrPose(spaceLocation.pose);
            if (spaceVelocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
            {
                linearVelocity = AzVector3FromXrVector3(spaceVelocity.linearVelocity);
            }
        }

        AZ::Quaternion poseOrientation = AZ::Quaternion::CreateIdentity();
        AZ::Vector3 angularVelocity = AZ::Vector3::CreateZero();
        if (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
        {
            poseOrientation = AzQuaternionFromXrPose(spaceLocation.pose);
            if (spaceVelocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
            {
                angularVelocity = AzVector3FromXrVector3(spaceVelocity.angularVelocity);
            }
        }

        PoseWithVelocities retPoseWithVelocities { 
            AZ::Transform::CreateFromQuaternionAndTranslation(poseOrientation, poseLocation),
            linearVelocity,
            angularVelocity
        };
        return AZ::Success(retPoseWithVelocities);
    }


    AZ::Outcome<bool, AZStd::string> ActionsManager::ApplyHapticVibrationAction(ActionHandle actionHandle,
        uint64_t durationNanos, float frequencyHz, float amplitude)
    {
        if (!actionHandle.IsValid())
        {
            return AZ::Failure("Invalid actionHandle!");
        }
        const auto actionIndex = actionHandle.GetIndex();

        // fire haptics using output action
        XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
        vibration.amplitude = AZStd::clamp(amplitude, 0.0f, 1.0f);
        vibration.duration = durationNanos;
        vibration.frequency = frequencyHz;
        XrHapticActionInfo hapticActionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
        hapticActionInfo.action = m_actions[actionIndex].m_xrAction;
        XrResult result = xrApplyHapticFeedback(m_xrSession, &hapticActionInfo, (const XrHapticBaseHeader*)&vibration);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }
        return AZ::Success(true);
    }


    AZ::Outcome<bool, AZStd::string> ActionsManager::StopHapticVibrationAction(ActionHandle actionHandle)
    {
        if (!actionHandle.IsValid())
        {
            return AZ::Failure("Invalid actionHandle!");
        }
        const auto actionIndex = actionHandle.GetIndex();

        // fire haptics using output action
        XrHapticActionInfo hapticActionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
        hapticActionInfo.action = m_actions[actionIndex].m_xrAction;
        XrResult result = xrStopHapticFeedback(m_xrSession, &hapticActionInfo);
        if (IsError(result))
        {
            return AZ::Failure(AZStd::string(GetResultString(result)));
        }
        return AZ::Success(true);
    }
    /// OpenXRActionsInterface overrides
    /////////////////////////////////////////////////
    
    
    void ActionsManager::RecreateXrActiveActionSets()
    {
        // Recreate and cache the XrActionActionSet list.
        m_xrActiveActionSets.clear();
        for (size_t i = 0; i < m_actionSets.size(); i++)
        {
            if (m_activeActionSets[i])
            {
                XrActiveActionSet activeActionSet{ m_actionSets[i].m_xrActionSet, XR_NULL_PATH };
                m_xrActiveActionSets.push_back(activeActionSet);
            }
        }
    }


    /////////////////////////////////////////////////
    /// AssetBus overrides
    void ActionsManager::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();

        AZ_Printf(LogName, "Successfully loaded the Action Sets Asset [%s].\n", asset.GetHint().c_str());
        
        // Finish ActionsManager initialization on the main loop.
        AZ::TickBus::QueueFunction([this, asset]()
            {
                m_actionSetAsset = asset;
                InitInternal();
            });
    }


    void ActionsManager::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();

        m_actionSetAsset = asset;

        // Report the error and do nothing.
        AZ_Assert(false, "Application failed to load The Action Sets asset. This application will keep running without user interaction support.");
        AZ_Error(LogName, false, "Application failed to load The Action Sets asset. This application will keep running without user interaction support.");
    }
    /// AssetBus overrides
    /////////////////////////////////////////////////

} // namespace OpenXRVk

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <openxr/openxr.h>

#include <OpenXRVk/OpenXRVkActionsInterface.h>
#include <OpenXRVk/OpenXRVkActionSetsAsset.h>

namespace OpenXRVk
{
    // Class that will help manage XrActionSet/XrAction.
    // All XrActionSet and XrActions will be created from an
    // asset created by the developer where all actions of interest
    // are instantiated.
    class ActionsManager final :
        public OpenXRActionsInterface::Registrar,
        private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ActionsManager, AZ::SystemAllocator);
        AZ_RTTI(ActionsManager, "{79E2B285-073B-4042-93ED-B92E6C819CA6}", IOpenXRActions);

        static constexpr char LogName[] = "OpenXRVkActionsManager";

        ActionsManager() = default;
        ~ActionsManager();

        //! @returns True if it was able to start the initialization process by queueing
        //!          asset loading of the Action Sets asset. This is a non-blocking call
        //!          and the complete initialization will happend asynchronously but it 
        //!          will be transparent to the Session.
        bool Init(XrInstance xrInstance, XrSession xrSession);

        //! Called by the Session each tick.
        bool SyncActions(XrTime predictedDisplayTime);

        //! Called by the Session when the event XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED is
        //! received.
        void OnInteractionProfileChanged();

        /////////////////////////////////////////////////
        /// OpenXRActionsInterface overrides
        AZStd::vector<AZStd::string> GetAllActionSets() const override;
        AZ::Outcome<bool, AZStd::string> GetActionSetState(const AZStd::string& actionSetName) const override;
        AZ::Outcome<bool, AZStd::string> SetActionSetState(const AZStd::string& actionSetName, bool activate) override;

        IOpenXRActions::ActionHandle GetActionHandle(const AZStd::string& actionSetName, const AZStd::string& actionName) const override;

        AZ::Outcome<bool, AZStd::string> GetActionStateBoolean(ActionHandle actionHandle) const override;
        AZ::Outcome<float, AZStd::string> GetActionStateFloat(ActionHandle actionHandle) const override;
        AZ::Outcome<AZ::Vector2, AZStd::string> GetActionStateVector2(ActionHandle actionHandle) const override;

        AZ::Outcome<bool, AZStd::string> SetBaseReferenceSpaceForPoseActions(const AZStd::string& visualizedSpaceName) override;
        const AZStd::string& GetBaseReferenceSpaceForPoseActions() const override;
        AZ::Outcome<AZ::Transform, AZStd::string> GetActionStatePose(ActionHandle actionHandle) const override;
        
        AZ::Outcome<PoseWithVelocities, AZStd::string> GetActionStatePoseWithVelocities(ActionHandle actionHandle) const override;

        AZ::Outcome<bool, AZStd::string> ApplyHapticVibrationAction(ActionHandle actionHandle, uint64_t durationNanos, float frequencyHz, float amplitude) override;
        AZ::Outcome<bool, AZStd::string> StopHapticVibrationAction(ActionHandle actionHandle) override;
        /// OpenXRActionsInterface overrides
        /////////////////////////////////////////////////

    private:
        // The following struct will be used at initialization only.
        // All the bindings for all actions will be called 
        struct SuggestedBindings
        {
            // Interaction Profile XrPath
            XrPath m_profileXrPath;
            // List of action bindings that will be registred for this profile.
            AZStd::vector<XrActionSuggestedBinding> m_suggestedBindingsList;
        };
        //! The key is the user friendly profile name.
        using SuggestedBindingsPerProfile = AZStd::unordered_map<AZStd::string, SuggestedBindings>;

        struct ActionInfo
        {
            AZStd::string m_name;
            XrActionType m_actionType = XR_ACTION_TYPE_MAX_ENUM;
            XrAction m_xrAction = XR_NULL_HANDLE;
            // Only valid if m_actionType is XR_ACTION_TYPE_POSE_INPUT
            XrSpace m_xrSpace = XR_NULL_HANDLE;
        };

        struct ActionSetInfo
        {
            AZStd::string m_name;
            XrActionSet m_xrActionSet;
            // The key is the name of the action.
            // The value is the index in @m_actions;
            AZStd::unordered_map<AZStd::string, IOpenXRActions::ActionHandle> m_actions;
        };

        // Called when m_actionSetAsset is ready.
        void InitInternal();

        //! Returns the Asset Cache relative product path of the
        //! ActionSets Asset.
        AZStd::string GetActionSetsAssetPath();

        //! Requests the Asset System to start loading the ActionSets Asset.
        //! @returns Success if the asset loading request was queued successfully.
        //! @remark Once OnAssetReady event is received, the XrActionSets and XrActions
        //!         will be created, along with their bindings etc.
        bool LoadActionSetAssetAsync();

        //! This function is called by InitInternal() to instantiate an XrActionSet from an OpenXRActionSetDescriptor.
        //! Additionally all XrActions for the XrActionSet will also be instantiated.
        //! @param interactionProfilesAsset The interaction profiles asset that owns the XrPath string data
        //!        that will be required to instantiate XrActions.
        //! @param actionSetDescriptor Contains all the data required to create an XrActionSet and all of its
        //!        XrActions
        //! @param suggestedBindingsPerProfileOut In this dictionary, to be used later by InitInternal(),
        //!        we collect all the Action Bindings for each Interaction Profile referenced by the
        //!        OpenXRActionSetsAsset.
        bool InitActionSet(const OpenXRInteractionProfilesAsset& interactionProfilesAsset,
            const OpenXRActionSetDescriptor& actionSetDescriptor,
            SuggestedBindingsPerProfile& suggestedBindingsPerProfileOut);


        //! Called by InitActionSet(...)
        bool AddActionToActionSet(const OpenXRInteractionProfilesAsset& interactionProfilesAsset,
            ActionSetInfo& actionSetInfo,
            const OpenXRActionDescriptor& actionDescriptor,
            SuggestedBindingsPerProfile& suggestedBindingsPerProfile);

        //! Called by AddActionToActionSet(...)
        XrAction CreateXrActionAndXrSpace(const ActionSetInfo& actionSetInfo,
            const OpenXRActionDescriptor& actionDescriptor, const XrActionType actionType, XrSpace& newXrActionSpace) const;

        //! Called by AddActionToActionSet(...)
        uint32_t AppendActionBindings(const OpenXRInteractionProfilesAsset& interactionProfilesAsset,
            const OpenXRActionDescriptor& actionDescriptor,
            XrAction xrAction,
            SuggestedBindingsPerProfile& suggestedBindingsPerProfile) const;

        //! The application can selectively activate or deactivate Action Sets.
        //! This is possible thanks to the public method SetActionSetState().
        //! Each time an Action Set is activated or deactivated this function is called
        //! to keep @m_xrActiveActionSets up to date, which allow us to call xrSyncActions
        //! with the ActionSets that should be active for the current frame.
        void RecreateXrActiveActionSets();


        /////////////////////////////////////////////////
        /// AssetBus overrides
        //! The reason we don't care for OnAssetReloaded() is because in OpenXR
        //! once a group of ActionSets have been attached to the session, they become
        //! immutable. To have different action sets, the session would have to be recreated.
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        /// AssetBus overrides
        /////////////////////////////////////////////////

        XrInstance m_xrInstance = XR_NULL_HANDLE;
        XrSession m_xrSession = XR_NULL_HANDLE;

        //! Loaded Asynchronously. Once this asset is ready, the real initialization of the
        //! OpenXR Actions and ActionSets will occur.
        //! This asset is loaded once and never changes.
        AZ::Data::Asset<OpenXRActionSetsAsset> m_actionSetAsset;

        // Updated each time SyncActions is called.
        XrTime m_predictedDisplaytime;

        AZStd::string m_baseReferenceSpaceName;
        XrSpace m_xrBaseReferenceSpace = XR_NULL_HANDLE;

        //! Each actionSet in this list is guaranteed to contain at least one valid action.
        AZStd::vector<ActionSetInfo> m_actionSets;
        //! This is a flat list of all actions across all actionSets.
        //! An IOpenXRActions::ActionHandle is actually an index into this list.
        AZStd::vector<ActionInfo> m_actions;

        //! This set is only useful for debugging purposes, we keep track
        //! of all the top level user paths so we can call xrGetCurrentInteractionProfile
        //! each time the Session receives the event XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED.
        mutable AZStd::unordered_set<AZStd::string> m_topLevelUserPaths;

        //! 32 action sets should be enough
        static constexpr uint32_t MaxActionSets = 32;
        // Each time ChangeActionSetState or ChangeActionSetsState is called
        // the following lists are updated:
        //! Each bit is an index in @m_actionsSets
        AZStd::bitset<MaxActionSets> m_activeActionSets;

        //! Here we cache the list of OpenXR native handles for active action sets.
        //! This list is recreated each time RecreateXrActiveActionSets() is called.
        AZStd::vector<XrActiveActionSet> m_xrActiveActionSets;

    };
}

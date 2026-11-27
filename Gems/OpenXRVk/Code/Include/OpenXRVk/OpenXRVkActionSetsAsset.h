/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <OpenXRVk/OpenXRVkInteractionProfilesAsset.h>

namespace OpenXRVk
{
    //! An Action Path Descriptor is nothing more than a tuple of three
    //! strings that identify a unique Input or Haptic control for a particular
    //! vendor equipment. The interesting point is that these strings MUST be limited
    //! to the unique names provided by an OpenXRInteractionProfileAsset.
    class OpenXRActionPathDescriptor final
    {
    public:
        AZ_RTTI(OpenXRActionPathDescriptor, "{F25D6382-C9E0-414B-A542-1758F5477D03}");
        virtual ~OpenXRActionPathDescriptor() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        AZStd::string GetEditorText() const;

        //! Should match an OpenXRInteractionProfileDescriptor::m_name
        AZStd::string m_interactionProfileName;
        
        //! Should match an OpenXRInteractionUserPathDescriptor::m_name
        AZStd::string m_userPathName;

        //! Should match an OpenXRInteractionComponentPathDescriptor::m_name
        AZStd::string m_componentPathName;

    private:
        AZ::Crc32 OnInteractionProfileSelected();
        AZStd::vector<AZStd::string> GetInteractionProfiles() const;

        AZ::Crc32 OnUserPathSelected();
        AZStd::vector<AZStd::string> GetUserPaths() const;

        AZ::Crc32 OnComponentPathSelected();
        AZStd::vector<AZStd::string> GetComponentPaths() const;
    };

    //! Describes a custom Action I/O that will be queried/driven
    //! by the application gameplay.
    class OpenXRActionDescriptor final
    {
    public:
        AZ_RTTI(OpenXRActionDescriptor, "{90BBF6F6-C7D6-4F64-B784-CE03F86DC36B}");
        virtual ~OpenXRActionDescriptor() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        AZStd::string GetEditorText() const;

        //! This name must be unique across all Actions listed in an Action Set.
        //! The content of this string is limited to the characters listed here:
        //! https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#well-formed-path-strings 
        AZStd::string m_name; // Regular char*

        //! User friendly name.
        AZStd::string m_localizedName; // UTF-8 string.

        //! Free form comment about this Action.
        AZStd::string m_comment;

        //! List of I/O action paths that will be bound to this action.
        //! The first action path in this list, determines what type of action paths
        //! can be added to the list. For example:
        //! If the first action path happens to be a boolean, then subsequent action paths
        //! can only be added if they can map to a boolean.
        //! Another important case is if the this is a haptic feedback action (Output), then
        //! subsequent action paths can only be of type haptic feedback actions.
        AZStd::vector<OpenXRActionPathDescriptor> m_actionPathDescriptors;
    };

    //! Describes a custom Action Set. All applications
    //! will have custom Action Sets because that's how developers define
    //! the gameplay I/O.
    class OpenXRActionSetDescriptor final
    {
    public:
        AZ_RTTI(OpenXRActionSetDescriptor, "{3A08BC1F-656F-441F-89C3-829F95B9B329}");
        virtual ~OpenXRActionSetDescriptor() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        AZStd::string GetEditorText() const;

        //! This name must be unique across all Action Sets listed in an Action Sets Asset.
        //! The content of this string is limited to the characters listed here:
        //! https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#well-formed-path-strings 
        AZStd::string m_name;

        //! User friendly name.
        AZStd::string m_localizedName; // UTF-8 string.
        
        //! Higher values mean higher priority.
        //! The priority is used by the OpenXR runtime in case several action sets
        //! use identical action paths and the highest priority will win the event.
        uint32_t m_priority = 0;
        
        //! Free form comment about this Action Set.
        AZStd::string m_comment;

        //! List of all actions under this Action Set.
        AZStd::vector<OpenXRActionDescriptor> m_actionDescriptors;
    };

    //! This asset defines a list of  OpenXR Action Sets that an application supports
    //! regarding inputs and haptics.
    class OpenXRActionSetsAsset final
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(OpenXRActionSetsAsset, AZ::SystemAllocator);
        AZ_RTTI(OpenXRActionSetsAsset, "{C2DEE370-6151-4701-AEA5-AEA3CA247CFF}", AZ::Data::AssetData);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr char s_assetTypeName[] = "OpenXR Action Sets Asset";
        static constexpr char s_assetExtension[] = "xractions";

        //! By referencing a particular Interaction Profiles asset, the actions
        //! exposed in this Action Sets asset will be limited to the vendor support
        //! profiles listed in the Interaction Profiles asset.
        AZ::Data::Asset<OpenXRInteractionProfilesAsset> m_interactionProfilesAsset;

        //! List of all Action Sets the application will work with.
        AZStd::vector<OpenXRActionSetDescriptor> m_actionSetDescriptors;

    private:
        AZ::Crc32 OnInteractionProfilesAssetChanged();
    };

    //! We need a custom asset handler because OpenXRActionSetsAsset contains a reference to another
    //! asset of type OpenXRInteractionProfilesAsset and we need to set a static singleton of type
    //! OpenXRInteractionProfilesAsset when the user is creating/editing an OpenXRActionSetsAsset with
    //! the Asset Editor. 
    class OpenXRActionSetsAssetHandler final
        : public AzFramework::GenericAssetHandler<OpenXRActionSetsAsset>
    {
    public:
        AZ_RTTI(OpenXRActionSetsAssetHandler, "{1C4A27E9-6768-4C59-9582-2A01A0DEC1D0}", AzFramework::GenericAssetHandler<OpenXRActionSetsAsset>);
        AZ_CLASS_ALLOCATOR(OpenXRActionSetsAssetHandler, AZ::SystemAllocator);
    
        static constexpr char LogName[] = "OpenXRInteractionProfilesAssetHandler";
    
        OpenXRActionSetsAssetHandler();
    
        // Called by the asset manager to perform actual asset load.
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;

        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;
    };
}// namespace OpenXRVk

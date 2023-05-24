/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class JobContext;
}

namespace AWSCore
{
    class AwsApiJobConfig;
}

namespace AWSCore
{
    static constexpr AZStd::string_view AWSMenuIdentifier = "o3de.menu.editor.aws";
    static constexpr AZStd::string_view EditorMainWindowMenuBarIdentifier = "o3de.menubar.editor.mainwindow";
    static constexpr AZStd::string_view HelpMenuIdentifier = "o3de.menu.editor.help";
    static constexpr AZStd::string_view EditorMainWindowActionContextIdentifier = "o3de.context.editor.mainwindow";
    static constexpr AZStd::string_view ActionContext = "o3de.context.editor.mainwindow";

    static constexpr const char AWS_MENU_TEXT[] = "&AWS";

    //! AWSCore request interface
    class AWSCoreRequests
        : public AZ::EBusTraits
    {
    public:
        //! Get the job context to make an AWS call.
        virtual AZ::JobContext* GetDefaultJobContext() = 0;

        // Returns the default client configuration setting to use as a starting point in AWS requests
        virtual AwsApiJobConfig* GetDefaultConfig() = 0;
        
        // Allow multiple threads to concurrently make requests
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };
    using AWSCoreRequestBus = AZ::EBus<AWSCoreRequests>;

    //! AWSCoreEditor request interface
    //! Used to check whether AWS feature gems activated
    class AWSCoreEditorRequests
        : public AZ::EBusTraits
    {
    public:
        virtual void CreateSubMenu(const AZStd::string& parentMenuIdentifier, const char* const menuDetails[], int sort) = 0;
        virtual void AddExternalLinkAction(const AZStd::string& menuIdentifier, const char* const actionDetails[], int sort) = 0;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };
    using AWSCoreEditorRequestBus = AZ::EBus<AWSCoreEditorRequests>;

    // Bus used to send notifications about AWSCore initialization
    class AWSCoreNotifications
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Sent when AWSCore has initialized the NativeSDK.
        //! SDK Shutdown on deactivate of AWSCoreSystemComponent.
        //! Dependent gems expected to shutdown/reset AWS NativeSDK service clients on corresponding system component reset.
        virtual void OnSDKInitialized() = 0;

        //! Sent when AWSCore is shutting down AWSNativeSDK
        virtual void OnSDKShutdownStarted() = 0;
    };
    using AWSCoreNotificationsBus = AZ::EBus<AWSCoreNotifications>;

} // namespace AWSCore

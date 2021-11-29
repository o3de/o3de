/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    //! Name of the AZ Trace window for source control messages
    static constexpr char SCC_WINDOW[] = "Source Control";

    enum SourceControlStatus
    {
        SCS_OpSuccess,          // No errors reported
        SCS_OpNotSupported,     // Operation not supported by the source control provider
        SCS_CertificateInvalid, // Trust certificate is invalid
        SCS_ProviderIsDown,     // source control provider is down
        SCS_ProviderNotFound,   // source control provider not found
        SCS_ProviderError,      // there was an error processing your request
        SCS_NUM_ERRORS,         // add errors above this enum
    };

    enum SourceControlFlags
    {
        SCF_OutOfDate       = (1 << 0), // the file was out of date
        SCF_Writeable       = (1 << 1), // the file is writable on disk
        SCF_MultiCheckOut   = (1 << 2), // this file allows multiple owners
        SCF_OtherOpen       = (1 << 3), // someone else has this file open
        SCF_PendingAdd      = (1 << 4), // file marked for add
        SCF_PendingDelete   = (1 << 5), // file marked for removal
        SCF_OpenByUser      = (1 << 6), // currently open for checkout / staging
        SCF_Tracked         = (1 << 7), // file is under source control
        SCF_PendingMove     = (1 << 8), // file marked for move
    };

    struct SourceControlFileInfo
    {
        SourceControlStatus m_status;
        unsigned int m_flags;
        AZStd::string m_filePath;
        AZStd::string m_StatusUser; // informational - this is the user that caused the above status.
        // secondary use of m_StatusUser is to signify that this file is being worked on by other users, simultaneously.

        SourceControlFileInfo()
            : m_status(SCS_ProviderIsDown)
            , m_flags((SourceControlFlags)0)
        {
        }

        SourceControlFileInfo(const char* fullFilePath)
            : m_status(SCS_ProviderIsDown)
            , m_filePath(fullFilePath)
            , m_flags((SourceControlFlags)0)
        {
        }

        SourceControlFileInfo(const SourceControlFileInfo& rhs)
        {
            *this = rhs;
        }

        SourceControlFileInfo(SourceControlFileInfo&& rhs)
            : m_status(rhs.m_status)
            , m_flags(rhs.m_flags)
            , m_filePath(AZStd::move(rhs.m_filePath))
            , m_StatusUser(AZStd::move(rhs.m_StatusUser))
        {
        }

        SourceControlFileInfo& operator=(const SourceControlFileInfo& rhs)
        {
            m_status = rhs.m_status;
            m_flags = rhs.m_flags;
            m_filePath = rhs.m_filePath;
            m_StatusUser = rhs.m_StatusUser;
            return *this;
        }

        bool CompareStatus(SourceControlStatus status) const { return m_status == status; }

        bool IsReadOnly() const { return !HasFlag(SCF_Writeable); }
        bool IsLockedByOther() const { return HasFlag(SCF_OtherOpen) && !HasFlag(SCF_MultiCheckOut); }
        bool IsManaged() const { return HasFlag(SCF_Tracked); }

        bool HasFlag(SourceControlFlags flag) const { return ((m_flags & flag) != 0); }
    };

    // use bind if you need additional context.
    typedef AZStd::function<void(bool success, SourceControlFileInfo info)> SourceControlResponseCallback;

    typedef AZStd::function<void(bool success, AZStd::vector<SourceControlFileInfo> info)> SourceControlResponseCallbackBulk;

    enum class SourceControlSettingStatus : int
    {
        Invalid,

        PERFORCE_BEGIN,
        Unset,
        None,
        Set,
        Config,
        PERFORCE_END,
    };

    struct SourceControlSettingInfo
    {
        SourceControlSettingStatus m_status = SourceControlSettingStatus::Invalid;
        AZStd::string m_value;
        AZStd::string m_context;

        SourceControlSettingInfo() = default;

        //! is this value actually present and usable?
        bool IsAvailable() const
        {
            return (m_status != SourceControlSettingStatus::Invalid) && (m_status != SourceControlSettingStatus::Unset) && (!m_value.empty());
        }

        //! Are we able to actually change this value without messing with global env or registry?
        bool IsSettable() const
        {
            if (m_status == SourceControlSettingStatus::Invalid)
            {
                return false;
            }

            return ((m_status == SourceControlSettingStatus::Unset) || (m_status == SourceControlSettingStatus::Set));
        }
    };

    typedef AZStd::function<void(const SourceControlSettingInfo& info)> SourceControlSettingCallback;

    enum class SourceControlState : int
    {
        Disabled,
        ConfigurationInvalid,
        Active,
    };

    //! SourceControlCommands
    //! This bus handles messages relating to source control commands
    //! source control commands are ASYNCHRONOUS
    //! do not block the main thread waiting for a response, it is not okay
    //! you will not get a message delivered unless you tick the tickbus anyway!
    class SourceControlCommands
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // there's only one source control listener right now
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;  // theres only one source control listener right now
        typedef AZStd::recursive_mutex MutexType;

        virtual ~SourceControlCommands() {}

        //! Get information on the file state
        virtual void GetFileInfo(const char* fullFilePath, const SourceControlResponseCallback& respCallback) = 0;

        //! Get information on the file state for multiple files.  Path(s) may contain wildcards
        virtual void GetBulkFileInfo(const AZStd::unordered_set<AZStd::string>& fullFilePaths, const SourceControlResponseCallbackBulk& respCallback) = 0;

        //! Attempt to make a file ready for editing
        virtual void RequestEdit(const char* fullFilePath, bool allowMultiCheckout, const SourceControlResponseCallback& respCallback) = 0;

        //! Attempt to make a set of files ready for editing
        virtual void RequestEditBulk(const AZStd::unordered_set<AZStd::string>& fullFilePaths, bool allowMultiCheckout, const SourceControlResponseCallbackBulk& respCallback) = 0;

        //! Attempt to delete a file
        virtual void RequestDelete(const char* fullFilePath, const SourceControlResponseCallback& respCallback) = 0;

        //! Attempt to delete a file
        //! @param skipReadOnly If source control is disabled and we're using the local file component, this will skip changes to files which are readonly
        virtual void RequestDeleteExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallback& respCallback) = 0;

        //! Attempt to delete multiple files.  Path may contain wildcards
        virtual void RequestDeleteBulk(const char* fullFilePath, const SourceControlResponseCallbackBulk& respCallback) = 0;

        //! Attempt to delete multiple files.  Path may contain wildcards
        //! @param skipReadOnly If source control is disabled and we're using the local file component, this will skip changes to files which are readonly
        virtual void RequestDeleteBulkExtended(const char* fullFilePath, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback) = 0;

        //! Attempt to revert a file
        virtual void RequestRevert(const char* fullFilePath, const SourceControlResponseCallback& respCallback) = 0;

        //! Attempt to get latest revision of a file
        virtual void RequestLatest(const char* fullFilePath, const SourceControlResponseCallback& respCallback) = 0;

        //! Attempt to rename or move a file
        virtual void RequestRename(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallback& respCallback) = 0;

        //! Attempt to rename or move a file
        //! @param skipReadOnly If source control is disabled and we're using the local file component, this will skip changes to files which are readonly
        virtual void RequestRenameExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallback& respCallback) = 0;

        //! Attempt to rename or move multiple files.  Path may contain wildcards
        virtual void RequestRenameBulk(const char* sourcePathFull, const char* destPathFull, const SourceControlResponseCallbackBulk& respCallback) = 0;

        //! Attempt to rename or move multiple files.  Path may contain wildcards
        //! @param skipReadOnly If source control is disabled and we're using the local file component, this will skip changes to files which are readonly
        virtual void RequestRenameBulkExtended(const char* sourcePathFull, const char* destPathFull, bool skipReadOnly, const SourceControlResponseCallbackBulk& respCallback) = 0;
    };

    using SourceControlCommandBus = AZ::EBus<SourceControlCommands>;

    //! SourceControlConnectionRequests
    //! This bus handles messages relating to source control connectivity
    class SourceControlConnectionRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~SourceControlConnectionRequests() {}

        //! Suspend / Resume source control operations
        virtual void EnableSourceControl(bool enable) = 0;

        //! Returns if source control operations are enabled
        virtual bool IsActive() const = 0;

        //! Enable or disable trust of an SSL connection
        virtual void EnableTrust(bool enable, AZStd::string fingerprint) = 0;

        //! Attempt to set connection setting 'key' to 'value'
        virtual void SetConnectionSetting(const char* key, const char* value, const SourceControlSettingCallback& respCallBack) = 0;

        //! Attempt to get connection setting by key
        virtual void GetConnectionSetting(const char* key, const SourceControlSettingCallback& respCallBack) = 0;

        //! Returns if source control is disabled, has invalid configurations, or enabled
        virtual SourceControlState GetSourceControlState() const { return SourceControlState::Disabled; }
    };

    using SourceControlConnectionRequestBus = AZ::EBus<SourceControlConnectionRequests>;

    //! SourceControlNotifications
    //! Outgoing messages from source control
    class SourceControlNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~SourceControlNotifications() {}

        //! Request to trust source control key with provided fingerprint
        virtual void RequestTrust(const char* /*fingerprint*/) {}

        //! Notify listeners that our connectivity state has changed
        virtual void ConnectivityStateChanged(const SourceControlState /*connected*/) {}
    };

    using SourceControlNotificationBus = AZ::EBus<SourceControlNotifications>;
}; // namespace AzToolsFramework

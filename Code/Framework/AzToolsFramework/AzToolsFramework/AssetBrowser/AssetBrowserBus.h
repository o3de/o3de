/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/string/string.h>
// warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
AZ_PUSH_DISABLE_WARNING(4127 4251, "-Wunknown-warning-option") 
#include <QIcon>
AZ_POP_DISABLE_WARNING

class QMimeData;
class QWidget;
class QImage;
class QMenu;

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }

    struct Uuid;
}

namespace AzQtComponents
{
    class StyledBusyLabel;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetSelectionModel;
        class AssetBrowserModel;
        class AssetBrowserEntry;

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserComponent
        //////////////////////////////////////////////////////////////////////////

        class AssetDatabaseLocationNotifications
            : public AZ::EBusTraits
        {
        public:
            //! Indicates that the Asset Database has been initialized
            virtual void OnDatabaseInitialized() = 0;
        };

        using AssetDatabaseLocationNotificationBus = AZ::EBus<AssetDatabaseLocationNotifications>;

        //! Sends requests to AssetBrowserComponent
        class AssetBrowserComponentRequests
            : public AZ::EBusTraits
        {
        public:

            // Only a single handler is allowed
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Request File Browser model
            virtual AssetBrowserModel* GetAssetBrowserModel() = 0;

            //! Returns true if entries were populated
            virtual bool AreEntriesReady() = 0;

            //! Spawn asset picker window
            //! @param selection Selection filter model for asset picker window
            //! @param parent Parent widget that previewer will be attached to
            virtual void PickAssets(AssetSelectionModel& selection, QWidget* parent) = 0;

            virtual AzQtComponents::StyledBusyLabel* GetStyledBusyLabel() = 0;
        };

        using AssetBrowserComponentRequestBus = AZ::EBus<AssetBrowserComponentRequests>;

        //! Sends notifications from AssetBrowserComponent
        class AssetBrowserComponentNotifications
            : public AZ::EBusTraits
        {
        public:
            //! Notifies when entries are physically populated in asset browser
            virtual void OnAssetBrowserComponentReady() {}
        };

        using AssetBrowserComponentNotificationBus = AZ::EBus<AssetBrowserComponentNotifications>;

        //////////////////////////////////////////////////////////////////////////
        // Interaction
        //////////////////////////////////////////////////////////////////////////

        //! This struct is used to respond about being able to open source files.
        //! See AssetBrowserInteractionNotifications::OpenSourceFileInEditor below
        struct SourceFileOpenerDetails
        {
            //! You provide a function to call (you may use std-bind to bind to your class) if your opener is chosen to handle the open operation
            typedef AZStd::function<void(const char* /*fullSourceFileName*/, const AZ::Uuid& /*source uuid*/)> SourceFileOpenerFunctionType;

            AZStd::string m_identifier; ///< choose something unique for your opener.  It may be used to restore state.  it will not be shown to user.

            //! m_displayText is used when more than one listener offers to open this kind of file and 
            //! we need the user to pick which one they want.  They will be offered all the available openers in a menu
            //! which shows this text, and the one they pick will get its SourceFileOpenerFunctionType called.
            AZStd::string m_displayText; 
            QIcon m_iconToUse; ///< optional.  Same as m_displayText.  Used when there's ambiguity.  If empty, no icon.

            //! This is the function to call.  If you fill a nullptr in here, then the default operating system behavior will be suppressed
            //! but no opener will be opened.  This will also cause the 'open' option in context menus to disappear if the only openers
            //! are nullptr ones.
            SourceFileOpenerFunctionType m_opener; 

            SourceFileOpenerDetails() = default;
            SourceFileOpenerDetails(const char* identifier, const char* displayText, QIcon icon, SourceFileOpenerFunctionType functionToCall)
                : m_identifier(identifier)
                , m_displayText(displayText)
                , m_iconToUse(icon)
                , m_opener(functionToCall) {}
        };

        typedef AZStd::vector<SourceFileOpenerDetails> SourceFileOpenerList;

        //! used by the API to (optionally) let systems describe details about source files
        //! see /ref AssetBrowserInteractionNotifications to see how it is used.
        //! The intended behavior of this is that listeners respond with a SourceFileDetails struct
        //! which has only the field(s) filled in which they can fill in, and the system combines all responses
        //! to fill in details from many systems.
        //! More fields may be added to SourceFileDetails as more systems require other kinds of details.
        struct SourceFileDetails
        {
            //! An openable path to a resource that can be used as the thumbnail for this type of file.
            //! This can be a Qt resource system string like ":/tools/something.png" for resources embedded in your 
            //! dlls, or an absolute path, or relative source asset path like "editor/icons/whatever.png"
            AZStd::string m_sourceThumbnailPath;

            //! Update this constructor or add more constructors if you add fields so that existing ones continue to work.
            SourceFileDetails(const char* thumbnailPath)
            {
                m_sourceThumbnailPath = thumbnailPath;
            }
            
            //! this is the function that will be used to "fold" multiple returned values from other results onto
            //! one canonical result containing all the result fields.  This function gets called repeatedly, once
            //! for every responding listener and can be used to avoid allocations.
            SourceFileDetails& operator=(SourceFileDetails&& other)
            {
                if (this != &other)
                {
                    if (m_sourceThumbnailPath.empty())
                    {
                        m_sourceThumbnailPath = AZStd::move(other.m_sourceThumbnailPath);
                    }
                }
                return *this;
            }

            ////////////////////////////////// boilerplate below here ///////////////////////////
            SourceFileDetails() = default;
            
            SourceFileDetails(const SourceFileDetails& other) = default;
            SourceFileDetails(SourceFileDetails&& other)
            {
                if (this != &other)
                {
                    *this = AZStd::move(other); // forward this to the below operator=&&
                }
            }
            SourceFileDetails& operator=(const SourceFileDetails& other) = default;
           
        };
 
        //! Bus for interaction with asset browser widget
        class AssetBrowserInteractionNotifications
            : public AZ::EBusTraits
        {
        public:
            using Bus = AZ::EBus<AssetBrowserInteractionNotifications>;
            typedef AZStd::recursive_mutex MutexType;

            //! Override this to get first attempt at handling these messages.   Higher priority goes first.
            virtual AZ::s32 GetPriority() const { return 0; }

            //! Notification that a context menu is about to be shown and offers an opportunity to add actions.
            virtual void AddContextMenuActions(QWidget* /*caller*/, QMenu* /*menu*/, const AZStd::vector<AssetBrowserEntry*>& /*entries*/) {};

            //! Implement AddSourceFileOpeners to provide your own editor for source files
            //! This gets called to collect the list of available openers for a file.
            //! Add your detail(s) to the openers list if you would like to be one of the options available to open the file.
            //! You can also add more than one to the list, or check the existing list to determine your behavior.
            //! If there is more than one in the list, the user will be given the choice of openers to use.
            //! If nobody responds (nobody adds their entry into the openers list), then the default operating system handler
            //! will be called (whatever that kind of file is associated with).
            virtual void AddSourceFileOpeners(const char* /*fullSourceFileName*/, const AZ::Uuid& /*sourceUUID*/, SourceFileOpenerList& /*openers*/) {}

            //! If you have an Asset Entry and would like to try to open it using the associated editor, you can use this bus to do so.
            //! Note that you can override this bus with a higher-than-zero priorit handler, and set alreadyHandled to true in your handler
            //! to prevent the default behavior from occuring.
            //! The default behavior is to call the above function for all handlers of that asset type, to gather the openers that can open it.
            //! following that, it either opens it with the opener (if there is only one) or prompts the user for which one to use.
            //! If no opener is present it tries to open it using the asset editor.
            //! finally, if its not a generic asset, it tries the operating system.
            virtual void OpenAssetInAssociatedEditor(const AZ::Data::AssetId& /*assetId*/, bool& /*alreadyHandled*/) {}

            //! Allows you to recognise the source files that your plugin cares about and provide information about the source file
            //! for display in the Asset Browser.  This allows you to override the default behavior if you wish to.
            //! note that you'll get SourceFileDetails for every file in view, and you should only return something if its YOUR
            //! kind of file that you have details to provide.
            virtual SourceFileDetails GetSourceFileDetails(const char* /*fullSourceFileName*/)
            {
                return SourceFileDetails();
            }

            //! required in order to sort the busses.
            inline bool Compare(const AssetBrowserInteractionNotifications* other) const
            {
                return GetPriority() > other->GetPriority();
            }
        };

        using AssetBrowserInteractionNotificationBus = AZ::EBus<AssetBrowserInteractionNotifications>;

        //////////////////////////////////////////////////////////////////////////
        // AssetBrowserModel
        //////////////////////////////////////////////////////////////////////////

        //! Sends requests to AssetBrowserModel
        class AssetBrowserModelRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            //! checks if AssetBrowserModel was updated at least once
            virtual bool IsLoaded() const = 0;

            virtual void BeginAddEntry(AssetBrowserEntry* parent) = 0;
            virtual void EndAddEntry(AssetBrowserEntry* parent) = 0;

            virtual void BeginRemoveEntry(AssetBrowserEntry* entry) = 0;
            virtual void EndRemoveEntry() = 0;
        };

        using AssetBrowserModelRequestBus = AZ::EBus<AssetBrowserModelRequests>;

        //! Notifies when AssetBrowserModel is updated
        class AssetBrowserModelNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            virtual void EntryAdded(const AssetBrowserEntry* /*entry*/) {}
            virtual void EntryRemoved(const AssetBrowserEntry* /*entry*/) {}
        };

        using AssetBrowserModelNotificationBus = AZ::EBus<AssetBrowserModelNotifications>;

        //! Sends requests to the Asset Browser view.
        class AssetBrowserViewRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            //! Requests the Asset Browser's view to select the given asset.
            //! @param assetID The asset to select.
            virtual void SelectProduct(AZ::Data::AssetId assetID) = 0;

            /**
            * Requests the Asset Browser's view to select the given asset.
            * \param assetPath The path (absolute or relative) of the asset to select.
            */
            virtual void SelectFileAtPath(const AZStd::string& assetPath) = 0;
            virtual void ClearFilter() = 0;

            virtual void Update() = 0;
        };
        using AssetBrowserViewRequestBus = AZ::EBus<AssetBrowserViewRequests>;

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.inl>

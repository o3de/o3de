/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContextBus.h>

#include <QKeySequence>

class QAction;
class QActionGroup;
class QKeySequence;
class QWidget;

namespace AzToolsFramework
{
    /// @name Reverse URLs.
    /// Used to identify common actions and override them when necessary.
    //@{
    static const AZ::Crc32 s_backAction = AZ_CRC_CE("org.o3de.action.common.back");
    static const AZ::Crc32 s_deleteAction = AZ_CRC_CE("org.o3de.action.common.delete");
    static const AZ::Crc32 s_duplicateAction = AZ_CRC_CE("org.o3de.action.common.duplicate");
    static const AZ::Crc32 s_nextComponentMode = AZ_CRC_CE("org.o3de.action.common.nextComponentMode");
    static const AZ::Crc32 s_previousComponentMode = AZ_CRC_CE("org.o3de.action.common.previousComponentMode");
    //@}

    /// Specific Action properties to be sent to a type implementing
    /// ActionOverrideRequests. The actions will be added and stored
    /// and will remain active until they are removed.
    struct ActionOverride
    {
        /// @cond
        ActionOverride() = default;
        /// @endcond

        /// Constructor to create an ActionOverride with all properties set.
        ActionOverride(
            const AZ::Crc32 uri, QKeySequence keySequence,
            const QString& title, const QString& statusTip,
            const AZStd::function<void()>& callback)
            : m_uri(uri), m_keySequence(keySequence)
            , m_title(title), m_statusTip(statusTip)
            , m_callback(callback) {}

        /// Set the URI associated with this ActionOverride.
        ActionOverride& SetUri(const AZ::Crc32 uri)
        {
            m_uri = uri;
            return *this;
        }

        /// Set the KeySequence (shortcut) associated with this ActionOverride.
        ActionOverride& SetKeySequence(const QKeySequence keySequence)
        {
            m_keySequence = keySequence;
            return *this;
        }

        /// Set the title associated with this ActionOverride.
        /// Will appear in the Edit menu bar.
        ActionOverride& SetTitle(const QString& title)
        { 
            m_title = title;
            return *this;
        }

        /// Set the tooltip associated with this ActionOverride.
        /// Will appear in the Edit menu bar with mouse hover.
        ActionOverride& SetTip(const QString& tip)
        {
            m_statusTip = tip;
            return *this;
        }

        /// Set the function to call for this ActionOverride.
        ActionOverride& SetCallback(const AZStd::function<void()>& callback)
        {
            m_callback = callback;
            return *this;
        }

        /// Set the Entity and Component this ActionOverride is associated with.
        /// \note This is important to call for actions associated with a specific entity/component.
        ActionOverride& SetEntityComponentIdPair(const AZ::EntityComponentIdPair& entityCompoentIdPair)
        {
            m_entityIdComponentPair = entityCompoentIdPair;
            return *this;
        }

        AZ::Crc32 m_uri; ///< Unique identifier for a particular action/shortcut.
        QKeySequence m_keySequence; ///< The shortcut bound to this particular action.
        QString m_title; ///< The title of the action to appear in the edit menu.
        QString m_statusTip; ///< The tool/status tip to appear when mouse is hovered over the edit menu action.
        AZStd::function<void()> m_callback = nullptr; ///< The operation to happen when the action is invoked.
        AZ::EntityComponentIdPair m_entityIdComponentPair { AZ::EntityId(), AZ::InvalidComponentId }; ///< The Entity and Component Id this
                                                                                                      ///< Action is associated with.
    };

    /// Helper to create common action _Back_
    inline ActionOverride CreateBackAction(const QString& title, const QString& tip, const AZStd::function<void()>& callback)
    {
        return ActionOverride(AzToolsFramework::s_backAction, QKeySequence(Qt::Key_Escape), title, tip, callback);
    }

    /// Helper to create common action _Delete_
    inline ActionOverride CreateDeleteAction(const QString& title, const QString& tip, const AZStd::function<void()>& callback)
    {
        return ActionOverride(AzToolsFramework::s_deleteAction, QKeySequence(Qt::Key_Delete), title, tip, callback);
    }

    /// Bus to allow Actions to be added/removed from the currently bound override widget.
    class ActionOverrideRequests
        : public AZ::EBusTraits
    {
    public:
        using BusIdType = AzFramework::EntityContextId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        /// Setup the override widget (set its parent) - usually MainWindow.
        virtual void SetupActionOverrideHandler(QWidget* parent) = 0;
        /// Teardown the override widget (clear its parent).
        virtual void TeardownActionOverrideHandler() = 0;
        /// Add a new ActionOverride to the override widget.
        virtual void AddActionOverride(const ActionOverride& actionOverride) = 0;
        /// Remove an ActionOverride using the ActionOverride URI.
        virtual void RemoveActionOverride(AZ::Crc32 actionOverrideUri) = 0;
        /// Remove all ActionOverrides that are currently bound.
        virtual void ClearActionOverrides() = 0;

    protected:
        ~ActionOverrideRequests() = default;
    };

    /// Type to inherit from to implement ActionOverrideRequests.
    using ActionOverrideRequestBus = AZ::EBus<ActionOverrideRequests>;

    /// Bus to allow default actions to be disabled/enabled and an override widget attached.
    /// (Note: Used to support custom actions/shortcuts in ComponentMode).
    class EditorActionRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        /// Allow default actions to be added to the Action Manager via a Bus call.
        virtual void AddActionViaBus(int id, QAction* action) = 0;
        /// Allow default actions to be added to the Action Manager via a Bus call and using the CRC id method.
        virtual void AddActionViaBusCrc(AZ::Crc32 id, QAction* action) = 0;
        /// Remove default actions added to the Action Manager via a Bus Call.
        virtual void RemoveActionViaBus(QAction* action) = 0;
        /// Enable all default actions that are active during the normal Editor state.
        virtual void EnableDefaultActions() = 0;
        /// Disable all default actions that are active during the normal Editor state.
        /// All Actions excluding those marked "Reserved" will be disabled (Undo/Redo/Save are excluded).
        virtual void DisableDefaultActions() = 0;
        /// Attach the override widget.
        /// The override widget will be responsible for 'first try' at all Actions
        /// that are routed through the Shortcut Dispatcher. If the override widget
        /// does not handle the action, it will pass through to normal handling.
        virtual void AttachOverride(QWidget* object) = 0;
        /// Detach the override widget.
        /// Stop the override widget from attempting to intercept events
        /// routed through the ShortcutDispatcher
        virtual void DetachOverride() = 0;

    protected:
        ~EditorActionRequests() = default;
    };

    /// Type to inherit from to implement EditorActionRequests.
    using EditorActionRequestBus = AZ::EBus<EditorActionRequests>;

    /// Provide operations that can be performed on the EditMenu.
    class EditorMenuRequests
        : public AZ::EBusTraits
    {
    public:
        /// Provide the ability to add an action to the edit menu.
        virtual void AddEditMenuAction(QAction* action) = 0;

        /// Add an action to the Editor menu.
        virtual void AddMenuAction(AZStd::string_view categoryId, QAction* action, bool addToToolsToolbar) = 0;

        /// (Re)populate the default EditMenu.
        /// Restore the EditMenu to its default state (the options available when first opening a level).
        virtual void RestoreEditMenuToDefault() = 0;

    protected:
        virtual ~EditorMenuRequests() = default;
    };

    /// Type to inherit from to implement EditorMenuRequests.
    using EditorMenuRequestBus = AZ::EBus<EditorMenuRequests>;

    /// Provides notifications from EditorMenu
    class EditorMenuNotifications
        : public AZ::EBusTraits
    {
    public:

        /// Dispatched when Editor's Tools menu is cleared.
        virtual void OnResetToolMenuItems() = 0;
    };

    /// Type to inherit to implement EditorMenuNotifications.
    using EditorMenuNotificationBus = AZ::EBus<EditorMenuNotifications>;
}

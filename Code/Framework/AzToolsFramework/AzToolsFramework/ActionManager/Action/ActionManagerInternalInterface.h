/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

class QAction;
class QWidget;

namespace AzToolsFramework
{
    using ActionManagerGetterResult = AZ::Outcome<AZStd::string, AZStd::string>;

    class ActionContextWidgetWatcher;
    class EditorAction;

    enum class ActionVisibility;

    //! ActionManagerInternalInterface
    //! Internal Interface to query implementation details for actions.
    class ActionManagerInternalInterface
    {
    public:
        AZ_RTTI(ActionManagerInternalInterface, "{2DCEB7AB-B07A-4085-B5AF-6EEB37439ED6}");

        //! Retrieve a QAction via its identifier.
        //! @param actionIdentifier The identifier for the action to retrieve.
        //! @return A raw pointer to the QAction, or nullptr if the action could not be found.
        virtual QAction* GetAction(const AZStd::string& actionIdentifier) = 0;

        //! Retrieve a QAction via its identifier (const version).
        //! @param actionIdentifier The identifier for the action to retrieve.
        //! @return A raw const pointer to the QAction, or nullptr if the action could not be found.
        virtual const QAction* GetActionConst(const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve an EditorAction via its identifier.
        //! @param actionIdentifier The identifier for the action to retrieve.
        //! @return A raw pointer to the EditorAction, or nullptr if the action could not be found.
        virtual EditorAction* GetEditorAction(const AZStd::string& actionIdentifier) = 0;

        //! Retrieve a EditorAction via its identifier (const version).
        //! @param actionIdentifier The identifier for the action to retrieve.
        //! @return A raw const pointer to the EditorAction, or nullptr if the action could not be found.
        virtual const EditorAction* GetEditorActionConst(const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve an ActionContextWidgetWatcher via its action context identifier.
        //! @param actionContextIdentifier The identifier for the action context whose ActionContextWidgetWatcher to retrieve.
        //! @return A raw pointer to the ActionContextWidgetWatcher, or nullptr if the ActionContextWidgetWatcher could not be found.
        virtual ActionContextWidgetWatcher* GetActionContextWidgetWatcher(const AZStd::string& actionContextIdentifier) = 0;

        //! Retrieve the Action's visibility property for Menus.
        //! @param actionIdentifier The identifier for the action to query.
        //! @return True if the actions should be hidden, false otherwise.
        virtual ActionVisibility GetActionMenuVisibility(const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve the Action's visibility property for ToolBars.
        //! @param actionIdentifier The identifier for the action to query.
        //! @return True if the actions should be hidden, false otherwise.
        virtual ActionVisibility GetActionToolBarVisibility(const AZStd::string& actionIdentifier) const = 0;

        //! Generate a QWidget from a Widget Action identifier.
        //! The WidgetAction will generate a new instance of the Widget and parent it to the widget provided.
        //! Is is on the caller to handle its lifetime correctly.
        //! @param widgetActionIdentifier The identifier for the widget action to retrieve.
        //! @return A raw pointer to the QWidget, or nullptr if the action could not be found.
        virtual QWidget* GenerateWidgetFromWidgetAction(const AZStd::string& widgetActionIdentifier) = 0;

        //! Update all actions that are parented to a specific action context.
        //! @param actionContextIdentifier The action context identifier for the context to update all actions for.
        virtual void UpdateAllActionsInActionContext(const AZStd::string& actionContextIdentifier) = 0;

        //! Completely reset the Action Manager from all items registered after initialization.
        //! Clears all Action Contexts, Actions and Widget Actions.
        //! Used in Unit tests to allow clearing the environment between runs.
        virtual void Reset() = 0;

    };

} // namespace AzToolsFramework

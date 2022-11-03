/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/function/function_base.h>

class QAction;
class QWidget;

namespace AzToolsFramework
{
    using ActionManagerGetterResult = AZ::Outcome<AZStd::string, AZStd::string>;

    class EditorAction;

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

        //! Retrieve whether an Action should be hidden from Menus when disabled.
        //! @param actionIdentifier The identifier for the action to query.
        //! @return True if the actions should be hidden, false otherwise.
        virtual bool GetHideFromMenusWhenDisabled(const AZStd::string& actionIdentifier) const = 0;

        //! Retrieve whether an Action should be hidden from ToolBars when disabled.
        //! @param actionIdentifier The identifier for the action to query.
        //! @return True if the actions should be hidden, false otherwise.
        virtual bool GetHideFromToolBarsWhenDisabled(const AZStd::string& actionIdentifier) const = 0;

        //! Generate a QWidget from a Widget Action identifier.
        //! The WidgetAction will generate a new instance of the Widget and parent it to the widget provided.
        //! Is is on the caller to handle its lifetime correctly.
        //! @param widgetActionIdentifier The identifier for the widget action to retrieve.
        //! @return A raw pointer to the QWidget, or nullptr if the action could not be found.
        virtual QWidget* GenerateWidgetFromWidgetAction(const AZStd::string& widgetActionIdentifier) = 0;

        //! Update all actions that are parented to a specific action context.
        //! @param actionContextIdentifier The action context identifier for the context to update all actions for.
        virtual void UpdateAllActionsInActionContext(const AZStd::string& actionContextIdentifier) = 0;
    };

} // namespace AzToolsFramework

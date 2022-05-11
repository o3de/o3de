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

#include <QWidget>

class QAction;

namespace AzToolsFramework
{
    using ActionManagerOperationResult = AZ::Outcome<void, AZStd::string>;

    //! ActionManagerInterface
    //! Interface to register and trigger actions in the Editor.
    class ActionManagerInterface
    {
    public:
        AZ_RTTI(ActionManagerInterface, "{2E2A421E-0842-4F90-9F5C-DDE0C4F820DE}");

        //! Register a new Action Context to the Action Manager.
        virtual ActionManagerOperationResult RegisterActionContext(
            QWidget* widget,
            const AZStd::string& identifier,
            const AZStd::string& name,
            const AZStd::string& parentIdentifier) = 0;

        //! Register a new Action to the Action Manager.
        virtual ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& identifier,
            const AZStd::string& name,
            const AZStd::string& description,
            const AZStd::string& category,
            const AZStd::string& iconPath,
            AZStd::function<void()> handler
        ) = 0;

        //! Trigger an Action via its identifier.
        virtual ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) = 0;

        //! Retrieve a QAction via its identifier.
        virtual QAction* GetAction(const AZStd::string& actionIdentifier) = 0;
        virtual const QAction* GetActionConst(const AZStd::string& actionIdentifier) = 0;
    };

} // namespace AzToolsFramework

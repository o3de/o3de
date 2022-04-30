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
    //! ActionManagerInterface
    //! Interface to register and trigger actions in the Editor.
    class ActionManagerInterface
    {
    public:
        AZ_RTTI(ActionManagerInterface, "{2E2A421E-0842-4F90-9F5C-DDE0C4F820DE}");

        //! Register a new Action Context to the Action Manager.
        virtual void RegisterActionContext(
            QWidget* parentWidget,
            AZStd::string_view identifier,
            AZStd::string_view name,
            AZStd::string_view parentIdentifier) = 0;

        //! Register a new Action to the Action Manager.
        virtual void RegisterAction(
            AZStd::string_view contextIdentifier,
            AZStd::string_view identifier,
            AZStd::string_view name,
            AZStd::string_view description,
            AZStd::string_view category,
            AZStd::string_view iconPath,
            AZStd::function<void()> handler
        ) = 0;

        //! Retrieve a QAction via its identifier.
        virtual QAction* GetAction(AZStd::string_view actionIdentifier) = 0;
    };

} // namespace AzToolsFramework

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>

namespace EditorPythonBindings
{
    //! ToolBarManagerRequestBus
    //! Bus to register and manage ToolBars in the Editor via Python.
    //! If writing C++ code, use the ToolBarManagerInterface instead.
    class ToolBarManagerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };

    using ToolBarManagerRequestBus = AZ::EBus<AzToolsFramework::ToolBarManagerInterface, ToolBarManagerRequests>;
} // namespace EditorPythonBindings

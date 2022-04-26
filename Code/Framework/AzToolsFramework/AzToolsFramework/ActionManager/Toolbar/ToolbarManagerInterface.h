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

namespace AzToolsFramework
{
    //! ToolbarManagerInterface
    //! Interface to register and manage toolbars in the Editor.
    class ToolbarManagerInterface
    {
    public:
        AZ_RTTI(ToolbarManagerInterface, "{205BB40C-0D61-4AF0-B6C6-6BE9EA36504E}");
    };

} // namespace AzToolsFramework

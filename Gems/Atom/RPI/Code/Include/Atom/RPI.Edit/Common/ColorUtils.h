/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyColorCtrl.hxx>

namespace AZ
{
    namespace RPI
    {
        namespace ColorUtils
        {
            //[GFX TODO][ATOM-4462] Replace this to use data driven color management system
            //! Return a ColorEditorConfiguration for editing a Linear sRGB color in sRGB space.
            AzToolsFramework::ColorEditorConfiguration GetLinearRgbEditorConfig();

        } // namespace PropertyColorConfigs
    } // namespace RPI
} // namespace AZ


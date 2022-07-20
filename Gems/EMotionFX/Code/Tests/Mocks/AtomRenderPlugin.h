/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>

namespace EMStudio
{
    // The Mock version of AtomRenderPlugin. It have the same name, class_id and type of the actual version.
    // The actual implementation of the AtomRenderPlugin is defined outside of EMotionFX gem. This Mock version
    // exists so we could load the default layouts without errors.
    class MockAtomRenderPlugin
        : public DockWidgetPlugin
    {
        enum
        {
            CLASS_ID = 0x32b0c04d
        };

        // Plugin information
        const char* GetName() const override
        {
            return "Atom Render Window";
        };
        uint32 GetClassID() const override
        {
            return CLASS_ID;
        };

        bool Init() override
        {
            return true;
        };

        EMStudioPlugin* Clone() const
        {
            return new MockAtomRenderPlugin();
        };

        EMStudioPlugin::EPluginType GetPluginType() const override
        {
            return EMStudioPlugin::PLUGINTYPE_RENDERING;
        };
    };

}

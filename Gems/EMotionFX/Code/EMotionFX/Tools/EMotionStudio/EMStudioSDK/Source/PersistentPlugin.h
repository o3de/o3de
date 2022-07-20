/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioPlugin.h>

namespace EMStudio
{
    //! Plugin that will be constructed along with the Animation Editor and survive its full life-cycle.
    //! Persistent plugins will not be destructed and re-created along with changing layouts.
    class PersistentPlugin
    {
    public:
        AZ_RTTI(PersistentPlugin, "{5A1715B1-4AAC-4DBE-B05F-F59D19EBF128}")
        virtual ~PersistentPlugin() = default;

        virtual const char* GetName() const = 0;

        virtual void Reflect([[maybe_unused]] AZ::ReflectContext* reflectContext) {}
        virtual PluginOptions* GetOptions() { return nullptr; }

        virtual void Update([[maybe_unused]] float timeDeltaInSeconds) {}
        virtual void Render([[maybe_unused]] EMotionFX::ActorRenderFlags renderFlags) {}
    };
} // namespace EMStudio

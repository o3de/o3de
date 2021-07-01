/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_RENDERUPDATECALLBACK_H
#define __EMSTUDIO_RENDERUPDATECALLBACK_H

#include "../EMStudioConfig.h"
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>


namespace EMStudio
{
    // forward declaration
    class RenderPlugin;

    // update and render callback
    class EMSTUDIO_API RenderUpdateCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(RenderUpdateCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        RenderUpdateCallback(RenderPlugin* plugin);

        void OnUpdateVisibilityFlags(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds);
        void OnUpdate(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds);
        void OnRender(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds);

        void SetEnableRendering(bool renderingEnabled);

    protected:
        bool            mEnableRendering;
        RenderPlugin*   mPlugin;
    };
} // namespace EMStudio


#endif

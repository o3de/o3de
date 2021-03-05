/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

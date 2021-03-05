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

#ifndef __EMSTUDIO_BENDSETUPINSTANCEDEBUGEVENTHANDLER_H
#define __EMSTUDIO_BENDSETUPINSTANCEDEBUGEVENTHANDLER_H

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "../StandardPluginsConfig.h"
#endif




namespace EMStudio
{
    class AnimGraphInstanceDebugEventHandler
        : public EMotionFX::AnimGraphInstanceEventHandler
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphInstanceDebugEventHandler, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        AnimGraphInstanceDebugEventHandler();
        virtual ~AnimGraphInstanceDebugEventHandler();

        void OnStateEnter(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state);
        void OnStateEntering(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state);
        void OnStateExit(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state);
        void OnStateEnd(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state);
        void OnStartTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition);
        void OnEndTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition);

    private:
    };
} // namespace EMStudio

#endif

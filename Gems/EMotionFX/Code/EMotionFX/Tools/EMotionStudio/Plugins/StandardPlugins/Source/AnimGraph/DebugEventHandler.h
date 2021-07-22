/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

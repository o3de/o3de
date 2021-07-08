/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <QApplication>
#include <Tests/ProvidesUI/AnimGraph/SimpleAnimGraphUIFixture.h>


namespace EMotionFX
{
    void SimpleAnimGraphUIFixture::SetUp()
    {
        UIFixture::SetUp();

        AZStd::string commandResult;
        MCore::CommandGroup group;

        // Create empty anim graph, add a motion node and a blend tree.
        group.AddCommandString(AZStd::string::format("CreateAnimGraph -animGraphID %d", m_animGraphId));
        group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 100 -yPos 100 -name testMotion",
            m_animGraphId, azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>().c_str()));
        group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 200 -yPos 100 -name testBlendTree",
            m_animGraphId, azrtti_typeid<BlendTree>().ToString<AZStd::string>().c_str()));
        group.AddCommandString(AZStd::string::format("AnimGraphCreateConnection -animGraphID %d -transitionType %s -sourceNode testMotion -targetNode testBlendTree",
            m_animGraphId, azrtti_typeid<AnimGraphStateTransition>().ToString<AZStd::string>().c_str()));

        // Create some paramters
        group.AddCommandString(AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -type \"%s\" -name bool_param",
            m_animGraphId, azrtti_typeid<BoolParameter>().ToString<AZStd::string>().c_str()));
        group.AddCommandString(AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -type \"%s\" -name float_param",
            m_animGraphId, azrtti_typeid<FloatSliderParameter>().ToString<AZStd::string>().c_str()));
        group.AddCommandString(AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -type \"%s\" -name vec2_param",
            m_animGraphId, azrtti_typeid<Vector2Parameter>().ToString<AZStd::string>().c_str()));

        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();
        m_animGraph = GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        EXPECT_NE(m_animGraph, nullptr) << "Cannot find newly created anim graph.";
        
        // Cache some local poitners.
        m_animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_NE(m_animGraphPlugin, nullptr) << "Anim graph plugin not found.";
    }

    void SimpleAnimGraphUIFixture::TearDown()
    {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        delete m_animGraph;
        UIFixture::TearDown();
    }
} // namespace EMotionFX

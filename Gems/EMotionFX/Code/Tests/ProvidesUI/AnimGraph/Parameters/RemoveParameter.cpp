/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <QApplication>
#include <QtTest>
#include "qtestsystem.h"

namespace EMotionFX
{
    class RemoveParameterFixture
        : public UIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            // Create empty anim graph and select it.
            const AZStd::string command = AZStd::string::format("CreateAnimGraph -animGraphID %d", m_animGraphId);
            AZStd::string commandResult;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
            m_animGraph = GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
            EXPECT_NE(m_animGraph, nullptr) << "Cannot find newly created anim graph.";

            auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
            EXPECT_NE(animGraphPlugin, nullptr) << "Anim graph plugin not found.";

            m_parameterWindow = animGraphPlugin->GetParameterWindow();
            EXPECT_NE(m_parameterWindow, nullptr) << "Anim graph parameter window is invalid.";
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            delete m_animGraph;
            UIFixture::TearDown();
        }

    public:
        const AZ::u32 m_animGraphId = 64;
        AnimGraph* m_animGraph = nullptr;
        EMStudio::ParameterWindow* m_parameterWindow = nullptr;
    };

    TEST_F(RemoveParameterFixture, RemoveParameterSimple)
    {
        AZStd::string commandString;
        AZStd::string commandResult;

        // Create parameter.
        const AZ::TypeId parameterTypeId = EMotionFX::ParameterFactory::GetValueParameterTypes()[0];
        AZStd::unique_ptr<Parameter> parameterPrototype(EMotionFX::ParameterFactory::Create(parameterTypeId));
        parameterPrototype->SetName("Parameter0");

        CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph, parameterPrototype.get());
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(commandString, commandResult)) << commandResult.c_str();
        EXPECT_EQ(m_animGraph->GetNumParameters(), 1);

        // Select and remove the parameter.
        m_parameterWindow->SelectParameters({parameterPrototype->GetName().c_str()});
        m_parameterWindow->OnRemoveSelected();

        // Verify that the parameter got correctly removed.
        EXPECT_EQ(m_animGraph->GetNumParameters(), 0) << "Removing the parameter failed.";
    }

    TEST_F(RemoveParameterFixture, RemoveAllSelectedParameters)
    {
        RecordProperty("test_case_id", "C1559137");

        AZStd::string commandString;
        AZStd::string commandResult;
        AZStd::vector<AZStd::string> parameterNames;

        // Create parameter for each of the available types.
        const AZStd::vector<AZ::TypeId> parameterTypeIds = EMotionFX::ParameterFactory::GetValueParameterTypes();
        for (const AZ::TypeId& parameterTypeId : parameterTypeIds)
        {
            const AZStd::string parameterName = AZStd::string::format("Parameter (Type=%s)", parameterTypeId.ToString<AZStd::string>().c_str());
            parameterNames.emplace_back(parameterName);

            AZStd::unique_ptr<Parameter> parameterPrototype(EMotionFX::ParameterFactory::Create(parameterTypeId));
            parameterPrototype->SetName(parameterName);

            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph, parameterPrototype.get());
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(commandString, commandResult)) << commandResult.c_str();
        }
        EXPECT_EQ(m_animGraph->GetNumParameters(), parameterTypeIds.size());

        // Select all parameters and remove them using the remove selected operation.
        m_parameterWindow->SelectParameters(parameterNames);
        m_parameterWindow->OnRemoveSelected();

        // Verify that all parameters got correctly removed.
        EXPECT_EQ(m_animGraph->GetNumParameters(), 0) << "Removing the parameter failed.";
    }
} // namespace EMotionFX

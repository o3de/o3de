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

#include <gtest/gtest.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <QApplication>
#include <QtTest>
#include "qtestsystem.h"
#include <Tests/ProvidesUI/AnimGraph/SimpleAnimGraphUIFixture.h>

namespace EMotionFX
{
    TEST_F(SimpleAnimGraphUIFixture, RemoveParametersTests)
    {
        // Check the parameters window
        EMStudio::ParameterWindow* parameterWindow = m_animGraphPlugin->GetParameterWindow();
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), m_animGraph->GetNumParameters()) << "Number of parameters displayed in the parameters window should be the same in the animgraph.";

        AZStd::string paramName;
        const AZ::u32 numIterations = 100;
        size_t numParams = m_animGraph->GetNumParameters();

        for (AZ::u32 i = 0; i < numIterations; ++i)
        {
            paramName = AZStd::string::format("testFloat%d", i);
            FloatParameter* floatParam = aznew FloatSliderParameter(paramName);
            floatParam->SetDefaultValue(0.0f);
            m_animGraph->AddParameter(floatParam);
        }

        numParams += numIterations;
        EXPECT_TRUE(m_animGraph->GetNumParameters() == numParams) << "The number of parameters should increase by 100 after adding 100 new float parameters to the anim graph.";

        parameterWindow->ClearParameters(false);
        AZStd::string result;

        EXPECT_EQ(m_animGraph->GetNumParameters(), 0) << "There should be no parameters after clear parameters.";
        EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(m_animGraph->GetNumParameters(), numParams) << "The number of parameters should recover to before clearing parameters.";
        EXPECT_EQ(parameterWindow->GetTopLevelItemCount(), m_animGraph->GetNumParameters()) << "Number of parameters displayed in the parameters window should be the same in the animgraph.";
        EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(result)) << result.c_str();
        EXPECT_EQ(m_animGraph->GetNumParameters(), 0) << "The number of parameters should zero after redo.";
    }
} // namespace EMotionFX

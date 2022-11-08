/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <QApplication>
#include <QComboBox>
#include <QtTest>
#include "qtestsystem.h"

namespace EMotionFX
{
    class AddParametersFixture
        : public UIFixture
        , public ::testing::WithParamInterface</*ParameterTypeIndex=*/int>
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();
            AZStd::string commandResult;

            // Create empty anim graph and select it.
            const AZStd::string command = AZStd::string::format("CreateAnimGraph -animGraphID %d", m_animGraphId);
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, commandResult)) << commandResult.c_str();
            m_animGraph = GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
            EXPECT_NE(m_animGraph, nullptr) << "Cannot find newly created anim graph.";
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
    };

    TEST_P(AddParametersFixture, AddParameters)
    {
        RecordProperty("test_case_id", "C1559138");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";

        EMStudio::ParameterWindow* parameterWindow = animGraphPlugin->GetParameterWindow();
        ASSERT_TRUE(parameterWindow) << "Anim graph parameter window is invalid.";

        // Normally users press the + button and a context menu appears with the options to either add a parameter or a group.
        // We are bypassing the context menu and directly call the add parameter slot.
        parameterWindow->OnAddParameter();

        // Create parameter window.
        QWidget* parameterCreate = FindTopLevelWidget("ParameterCreateEditWidget");
        ASSERT_NE(parameterCreate, nullptr) << "Cannot find anim graph parameter create/edit widget. Is the anim graph selected?";
        auto parameterCreateWidget = qobject_cast<EMStudio::ParameterCreateEditWidget*>(parameterCreate);

        // Set the parameter type using the combo box.
        QComboBox* valueTypeComboBox = parameterCreateWidget->GetValueTypeComboBox();
        const int row = GetParam();
        valueTypeComboBox->setCurrentIndex(row);
        EXPECT_EQ(row, valueTypeComboBox->currentIndex()) << "Changing the value type failed. Out of bounds?";

        // Verify if the type ids match.
        const QByteArray parameterTypeIdString = valueTypeComboBox->itemData(row, Qt::UserRole).toString().toUtf8();
        const AZ::TypeId parameterTypeId = AZ::TypeId::CreateString(parameterTypeIdString.data(), parameterTypeIdString.size());
        EXPECT_FALSE(parameterTypeId.IsNull()) << "Selected parameter type is invalid.";
        EXPECT_EQ(parameterTypeId, EMotionFX::ParameterFactory::GetValueParameterTypes()[row])
            << "The parameter type id from the combo box do not match the type ids from the parameter factory.";

        // Accept the dialog (this creates the actual parameter object in the anim graph).
        parameterCreateWidget->accept();
        
        // Verify the parameter in the anim graph.
        EXPECT_EQ(m_animGraph->GetNumParameters(), 1) << "Parameter creation failed. We should end up with exactly one parameter.";
        const Parameter* parameter = m_animGraph->FindParameter(0);
        EXPECT_NE(parameter, nullptr) << "The parameter we created should be valid.";
        EXPECT_EQ(parameter->RTTI_GetType(), parameterTypeId)
            << "The type of the created parameter does not match the selected type in the dialog.";
    }

    std::vector<int> GetValueParameterTypeIndices()
    {
        // We cannot call EMotionFX::ParameterFactory::GetValueParameterTypes().size() here as the Az is not initialized yet.
        const int numValueParameterTypes = 13;

        std::vector<int> result(numValueParameterTypes);
        std::iota (std::begin(result), std::end(result), 0);
        return result;
    }

    INSTANTIATE_TEST_CASE_P(AddParameters,
        AddParametersFixture,
        ::testing::ValuesIn(GetValueParameterTypeIndices()));
} // namespace EMotionFX

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

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectActionManager.h>
#include <Editor/InputDialogValidatable.h>
#include <Editor/SimulatedObjectHelpers.h>
#include <QWidget>

namespace EMStudio
{
    void SimulatedObjectActionManager::OnAddNewObjectAndAddJoints(EMotionFX::Actor* actor, const QModelIndexList& selectedJoints, bool addChildJoints, QWidget* parent)
    {
        if (!actor)
        {
            AZ_Error("EMotionFX", false, "Cannot add new simulated object. Actor is not valid.");
            return;
        }

        InputDialogValidatable* inputDialog = new InputDialogValidatable(parent, /*labelText=*/"Name:");
        inputDialog->setWindowTitle("New simulated object name");
        inputDialog->setMinimumWidth(300);
        inputDialog->setObjectName("EMFX.SimulatedObjectActionManager.SimulatedObjectDialog");

        inputDialog->SetValidatorFunc([inputDialog, actor]() {
            EMotionFX::SimulatedObjectSetup* simulatedObjectSetup = actor->GetSimulatedObjectSetup().get();
            if (simulatedObjectSetup)
            {
                return simulatedObjectSetup->IsSimulatedObjectNameUnique(inputDialog->GetText().toUtf8().data(), /*checkedSimulatedObject=*/nullptr);
            }

            return false;
        });

        EMStudio::InputDialogValidatable::connect(inputDialog, &QDialog::finished, [actor, selectedJoints, inputDialog, addChildJoints](int resultCode) {
            inputDialog->deleteLater();

            if (resultCode == QDialog::Rejected)
            {
                return;
            }
            const AZStd::string commadGroupName = AZStd::string::format("Add simulated object%s", selectedJoints.empty() ? "" : " and joints");
            MCore::CommandGroup commandGroup(commadGroupName);

            EMotionFX::SimulatedObjectHelpers::AddSimulatedObject(actor->GetID(), inputDialog->GetText().toUtf8().data(), &commandGroup);
            EMotionFX::SimulatedObjectHelpers::AddSimulatedJoints(selectedJoints, actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), addChildJoints, &commandGroup);

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str())
            }
        });

        inputDialog->open();
    }
} // namespace EMStudio

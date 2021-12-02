/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ResetSettingsDialog.h"

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace EMStudio
{
    // Iterates through the objects in one of the Manager classes, and returns
    // true if there is at least one object that is not owned by the runtime
    template<class ManagerType, typename GetNumFunc, typename GetEntityFunc>
    bool HasEntityInEditor(const ManagerType& manager, const GetNumFunc& getNumEntitiesFunc, const GetEntityFunc& getEntityFunc)
    {
        const size_t numEntities = (manager.*getNumEntitiesFunc)();
        for (size_t i = 0; i < numEntities; ++i)
        {
            const auto& entity = (manager.*getEntityFunc)(i);
            if (!entity->GetIsOwnedByRuntime())
            {
                return true;
            }
        }
        return false;
    }

    ResetSettingsDialog::ResetSettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        // update title of the dialog
        setWindowTitle("Reset Workspace");

        QVBoxLayout* vLayout = new QVBoxLayout(this);
        vLayout->setAlignment(Qt::AlignTop);

        setObjectName("StyledWidgetDark");

        QLabel* topLabel = new QLabel("<b>Select one or more items that you want to reset:</b>");
        topLabel->setStyleSheet("background-color: rgb(40, 40, 40); padding: 6px;");
        topLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        vLayout->addWidget(topLabel);
        vLayout->setMargin(0);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(5);
        layout->setSpacing(4);
        vLayout->addLayout(layout);

        m_actorCheckbox = new QCheckBox("Actors");
        m_actorCheckbox->setObjectName("EMFX.ResetSettingsDialog.Actors");
        const bool hasActors = EMotionFX::GetActorManager().GetNumActors() > 0;
        m_actorCheckbox->setChecked(hasActors);
        m_actorCheckbox->setDisabled(!hasActors);

        m_motionCheckbox = new QCheckBox("Motions");
        m_motionCheckbox->setObjectName("EMFX.ResetSettingsDialog.Motions");
        const bool hasMotions = HasEntityInEditor(
            EMotionFX::GetMotionManager(), &EMotionFX::MotionManager::GetNumMotions, &EMotionFX::MotionManager::GetMotion);
        m_motionCheckbox->setChecked(hasMotions);
        m_motionCheckbox->setDisabled(!hasMotions);

        m_motionSetCheckbox = new QCheckBox("Motion Sets");
        m_motionSetCheckbox->setObjectName("EMFX.ResetSettingsDialog.MotionSets");
        const bool hasMotionSets = HasEntityInEditor(
            EMotionFX::GetMotionManager(), &EMotionFX::MotionManager::GetNumMotionSets, &EMotionFX::MotionManager::GetMotionSet);
        m_motionSetCheckbox->setChecked(hasMotionSets);
        m_motionSetCheckbox->setDisabled(!hasMotionSets);

        m_animGraphCheckbox = new QCheckBox("Anim Graphs");
        m_animGraphCheckbox->setObjectName("EMFX.ResetSettingsDialog.AnimGraphs");
        const bool hasAnimGraphs = HasEntityInEditor(
            EMotionFX::GetAnimGraphManager(), &EMotionFX::AnimGraphManager::GetNumAnimGraphs, &EMotionFX::AnimGraphManager::GetAnimGraph);
        m_animGraphCheckbox->setChecked(hasAnimGraphs);
        m_animGraphCheckbox->setDisabled(!hasAnimGraphs);

        layout->addWidget(m_actorCheckbox);
        layout->addWidget(m_motionCheckbox);
        layout->addWidget(m_motionSetCheckbox);
        layout->addWidget(m_animGraphCheckbox);

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        vLayout->addWidget(buttonBox);
    }

    bool ResetSettingsDialog::IsActorsChecked() const
    {
        return m_actorCheckbox->isChecked();
    }

    bool ResetSettingsDialog::IsMotionsChecked() const
    {
        return m_motionCheckbox->isChecked();
    }

    bool ResetSettingsDialog::IsMotionSetsChecked() const
    {
        return m_motionSetCheckbox->isChecked();
    }

    bool ResetSettingsDialog::IsAnimGraphsChecked() const
    {
        return m_animGraphCheckbox->isChecked();
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/moc_ResetSettingsDialog.cpp>

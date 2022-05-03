/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QSettings>
#include <EMStudio/AtomRenderPlugin.h>
#include <EMStudio/AnimViewportToolBar.h>
#include <EMStudio/AnimViewportRequestBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <AzCore/std/string/string.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>


namespace EMStudio
{
    AnimViewportToolBar::AnimViewportToolBar(AtomRenderPlugin* plugin, QWidget* parent)
        : QToolBar(parent)
        , m_plugin(plugin)
    {
        AzQtComponents::ToolBar::addMainToolBarStyle(this);

        // Add the manipulator actions
        QActionGroup* manipulatorGroup = new QActionGroup(this);
        manipulatorGroup->setExclusive(true);
        manipulatorGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
        m_manipulatorActions[RenderOptions::TRANSLATE] = addAction(QIcon(":/EMotionFXAtom/Translate.svg"), "Translate");
        m_manipulatorActions[RenderOptions::ROTATE] = addAction(QIcon(":/EMotionFXAtom/Rotate.svg"), "Rotate");
        m_manipulatorActions[RenderOptions::SCALE] = addAction(QIcon(":/EMotionFXAtom/Scale.svg"), "Scale");
        for (size_t i = RenderOptions::ManipulatorMode::TRANSLATE; i < RenderOptions::ManipulatorMode::NUM_MODES; ++i)
        {
            m_manipulatorActions[i]->setCheckable(true);
            connect(
                m_manipulatorActions[i], &QAction::triggered, this,
                [this, i]()
                {
                    const RenderOptions::ManipulatorMode mode =
                        m_manipulatorActions[i]->isChecked() ? RenderOptions::ManipulatorMode(i) : RenderOptions::ManipulatorMode::SELECT;
                    m_plugin->GetRenderOptions()->SetManipulatorMode(mode);
                    m_plugin->SetManipulatorMode(mode);
                });
            manipulatorGroup->addAction(m_manipulatorActions[i]);
        }
        addSeparator();

        // Add the render view options button
        QToolButton* renderOptionsButton = new QToolButton(this);
        {
            QMenu* contextMenu = new QMenu(renderOptionsButton);

            renderOptionsButton->setText("Render Options");
            renderOptionsButton->setMenu(contextMenu);
            renderOptionsButton->setPopupMode(QToolButton::InstantPopup);
            renderOptionsButton->setVisible(true);
            renderOptionsButton->setIcon(QIcon(":/EMotionFXAtom/Visualization.svg"));
            addWidget(renderOptionsButton);

            CreateViewOptionEntry(contextMenu, "Solid", EMotionFX::ActorRenderFlagIndex::SOLID);
            CreateViewOptionEntry(contextMenu, "Wireframe", EMotionFX::ActorRenderFlagIndex::WIREFRAME);
            // [EMFX-TODO] Add those option once implemented.
            // CreateViewOptionEntry(contextMenu, "Lighting", EMotionFX::ActorRenderFlagIndex::RENDER_LIGHTING);
            // CreateViewOptionEntry(contextMenu, "Backface Culling", EMotionFX::ActorRenderFlagIndex::RENDER_BACKFACECULLING);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Vertex Normals", EMotionFX::ActorRenderFlagIndex::VERTEXNORMALS);
            CreateViewOptionEntry(contextMenu, "Face Normals", EMotionFX::ActorRenderFlagIndex::FACENORMALS);
            CreateViewOptionEntry(contextMenu, "Tangents", EMotionFX::ActorRenderFlagIndex::TANGENTS);
            CreateViewOptionEntry(contextMenu, "Actor Bounding Boxes", EMotionFX::ActorRenderFlagIndex::AABB);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Line Skeleton", EMotionFX::ActorRenderFlagIndex::LINESKELETON);
            CreateViewOptionEntry(contextMenu, "Solid Skeleton", EMotionFX::ActorRenderFlagIndex::SKELETON);
            CreateViewOptionEntry(contextMenu, "Joint Names", EMotionFX::ActorRenderFlagIndex::NODENAMES);
            CreateViewOptionEntry(contextMenu, "Joint Orientations", EMotionFX::ActorRenderFlagIndex::NODEORIENTATION);
            // [EMFX-TODO] Add those option once implemented.
            // CreateViewOptionEntry(contextMenu, "Actor Bind Pose", EMotionFX::ActorRenderFlagIndex::RENDER_ACTORBINDPOSE);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Hit Detection Colliders", EMotionFX::ActorRenderFlagIndex::HITDETECTION_COLLIDERS, true,
                ":/EMotionFXAtom/HitDetection.svg");
            CreateViewOptionEntry(contextMenu, "Ragdoll Colliders", EMotionFX::ActorRenderFlagIndex::RAGDOLL_COLLIDERS, true,
                ":/EMotionFXAtom/RagdollCollider.svg");
            CreateViewOptionEntry(contextMenu, "Ragdoll Joint Limits", EMotionFX::ActorRenderFlagIndex::RAGDOLL_JOINTLIMITS, true,
                ":/EMotionFXAtom/RagdollJointLimit.svg");
            CreateViewOptionEntry(contextMenu, "Cloth Colliders", EMotionFX::ActorRenderFlagIndex::CLOTH_COLLIDERS, true,
                ":/EMotionFXAtom/Cloth.svg");
            CreateViewOptionEntry(contextMenu, "Simulated Object Colliders", EMotionFX::ActorRenderFlagIndex::SIMULATEDOBJECT_COLLIDERS, true,
                ":/EMotionFXAtom/SimulatedObjectCollider.svg");
            CreateViewOptionEntry(contextMenu, "Simulated Joints", EMotionFX::ActorRenderFlagIndex::SIMULATEJOINTS);

            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Motion Extraction", EMotionFX::ActorRenderFlagIndex::MOTIONEXTRACTION);
            CreateViewOptionEntry(contextMenu, "Debug Rendering", EMotionFX::ActorRenderFlagIndex::EMFX_DEBUG);
        }

        // Add the camera button
        QToolButton* cameraButton = new QToolButton(this);
        {
            QMenu* cameraMenu = new QMenu(cameraButton);

            // Add the camera option
            const AZStd::vector<AZStd::pair<RenderOptions::CameraViewMode, AZStd::string>> cameraOptionNames = {
                { RenderOptions::CameraViewMode::FRONT, "Front" },
                { RenderOptions::CameraViewMode::BACK, "Back" },
                { RenderOptions::CameraViewMode::TOP, "Top" },
                { RenderOptions::CameraViewMode::BOTTOM, "Bottom" },
                { RenderOptions::CameraViewMode::LEFT, "Left" },
                { RenderOptions::CameraViewMode::RIGHT, "Right" },
            };

            for (const auto& pair : cameraOptionNames)
            {
                RenderOptions::CameraViewMode mode = pair.first;
                cameraMenu->addAction(
                    pair.second.c_str(),
                    [this, mode]()
                    {
                        m_plugin->GetRenderOptions()->SetCameraViewMode(mode);
                        AnimViewportRequestBus::Broadcast(&AnimViewportRequestBus::Events::UpdateCameraViewMode, mode);
                    });
            }

            cameraMenu->addSeparator();
            cameraMenu->addAction(
                "Reset Camera",
                [this]()
                {
                    m_plugin->GetRenderOptions()->SetCameraViewMode(RenderOptions::CameraViewMode::DEFAULT);
                    AnimViewportRequestBus::Broadcast(
                        &AnimViewportRequestBus::Events::UpdateCameraViewMode, RenderOptions::CameraViewMode::DEFAULT);
                });

            cameraMenu->addSeparator();
            m_followCharacterAction = cameraMenu->addAction("Follow Character");
            m_followCharacterAction->setCheckable(true);
            m_followCharacterAction->setChecked(false);
            connect(m_followCharacterAction, &QAction::triggered, this,
                [this]()
                {
                    m_plugin->GetRenderOptions()->SetCameraFollowUp(m_followCharacterAction->isChecked());
                    AnimViewportRequestBus::Broadcast(
                        &AnimViewportRequestBus::Events::UpdateCameraFollowUp, m_followCharacterAction->isChecked());
                    ;
                });

            connect(cameraMenu, &QMenu::aboutToShow, this,
                [this]()
                {
                    m_followCharacterAction->setChecked(m_plugin->GetRenderOptions()->GetCameraFollowUp());
                });

            cameraButton->setMenu(cameraMenu);
            cameraButton->setText("Camera Option");
            cameraButton->setPopupMode(QToolButton::InstantPopup);
            cameraButton->setVisible(true);
            cameraButton->setIcon(QIcon(":/EMotionFXAtom/Camera_category.svg"));
            addWidget(cameraButton);
        }

        LoadSettings();
    }

    AnimViewportToolBar::~AnimViewportToolBar()
    {
    }

    void AnimViewportToolBar::CreateViewOptionEntry(
        QMenu* menu, const char* menuEntryName, AZ::u8 actionIndex, bool visible, const char* iconFileName)
    {
        QAction* action = menu->addAction(
            menuEntryName,
            [this, actionIndex]()
            {
                m_plugin->GetRenderOptions()->ToggerRenderFlag(actionIndex);
                // Send the reset camera event.
                AnimViewportRequestBus::Broadcast(&AnimViewportRequestBus::Events::UpdateRenderFlags, m_plugin->GetRenderOptions()->GetRenderFlags());
            });
        action->setCheckable(true);
        action->setVisible(visible);

        if (iconFileName)
        {
            action->setIcon(QIcon(iconFileName));
        }

        m_renderActions[actionIndex] = action;
    }

    void AnimViewportToolBar::LoadSettings()
    {
        const RenderOptions* renderOptions = m_plugin->GetRenderOptions();

        const bool isChecked = renderOptions->GetCameraFollowUp();
        m_followCharacterAction->setChecked(isChecked);
        AnimViewportRequestBus::Broadcast(&AnimViewportRequestBus::Events::UpdateCameraFollowUp, isChecked);

        RenderOptions::ManipulatorMode mode = renderOptions->GetManipulatorMode();
        m_plugin->SetManipulatorMode(mode);
        if (mode != RenderOptions::ManipulatorMode::SELECT)
        {
            m_manipulatorActions[mode]->setChecked(true);
        }

        const EMotionFX::ActorRenderFlags renderFlags = renderOptions->GetRenderFlags();
        for (uint8 i = 0; i < EMotionFX::ActorRenderFlagIndex::NUM_RENDERFLAGINDEXES; ++i)
        {
            QAction* action = m_renderActions[i];
            if (action)
            {
                action->setChecked(EMotionFX::ActorRenderFlagUtil::CheckBit(renderFlags, i));
            }
        }
    }
} // namespace EMStudio

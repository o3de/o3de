/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolButton>
#include <QMenu>
#include <QSettings>
#include <EMStudio/AnimViewportToolBar.h>
#include <EMStudio/AnimViewportRequestBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <AzCore/std/string/string.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>

namespace EMStudio
{
    AnimViewportToolBar::AnimViewportToolBar(QWidget* parent)
        : QToolBar(parent)
    {
        AzQtComponents::ToolBar::addMainToolBarStyle(this);

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

            CreateViewOptionEntry(contextMenu, "Solid", EMotionFX::ActorRenderFlag::RENDER_SOLID);
            CreateViewOptionEntry(contextMenu, "Wireframe", EMotionFX::ActorRenderFlag::RENDER_WIREFRAME);
            // [EMFX-TODO] Add those option once implemented.
            // CreateViewOptionEntry(contextMenu, "Lighting", EMotionFX::ActorRenderFlag::RENDER_LIGHTING);
            // CreateViewOptionEntry(contextMenu, "Backface Culling", EMotionFX::ActorRenderFlag::RENDER_BACKFACECULLING);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Vertex Normals", EMotionFX::ActorRenderFlag::RENDER_VERTEXNORMALS);
            CreateViewOptionEntry(contextMenu, "Face Normals", EMotionFX::ActorRenderFlag::RENDER_FACENORMALS);
            CreateViewOptionEntry(contextMenu, "Tangents", EMotionFX::ActorRenderFlag::RENDER_TANGENTS);
            CreateViewOptionEntry(contextMenu, "Actor Bounding Boxes", EMotionFX::ActorRenderFlag::RENDER_AABB);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Line Skeleton", EMotionFX::ActorRenderFlag::RENDER_LINESKELETON);
            CreateViewOptionEntry(contextMenu, "Solid Skeleton", EMotionFX::ActorRenderFlag::RENDER_SKELETON);
            CreateViewOptionEntry(contextMenu, "Joint Names", EMotionFX::ActorRenderFlag::RENDER_NODENAMES);
            CreateViewOptionEntry(contextMenu, "Joint Orientations", EMotionFX::ActorRenderFlag::RENDER_NODEORIENTATION);
            // [EMFX-TODO] Add those option once implemented.
            // CreateViewOptionEntry(contextMenu, "Actor Bind Pose", EMotionFX::ActorRenderFlag::RENDER_ACTORBINDPOSE);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Hit Detection Colliders", EMotionFX::ActorRenderFlag::RENDER_HITDETECTION_COLLIDERS);
            CreateViewOptionEntry(contextMenu, "Ragdoll Colliders", EMotionFX::ActorRenderFlag::RENDER_RAGDOLL_COLLIDERS);
            CreateViewOptionEntry(contextMenu, "Ragdoll Joint Limits", EMotionFX::ActorRenderFlag::RENDER_RAGDOLL_JOINTLIMITS);
            CreateViewOptionEntry(contextMenu, "Cloth Colliders", EMotionFX::ActorRenderFlag::RENDER_CLOTH_COLLIDERS);
            CreateViewOptionEntry(contextMenu, "Simulated Object Colliders", EMotionFX::ActorRenderFlag::RENDER_SIMULATEDOBJECT_COLLIDERS);
            CreateViewOptionEntry(contextMenu, "Simulated Joints", EMotionFX::ActorRenderFlag::RENDER_SIMULATEJOINTS);
        }

        // Add the camera button
        QToolButton* cameraButton = new QToolButton(this);
        {
            QMenu* cameraMenu = new QMenu(cameraButton);

            // Add the camera option
            const AZStd::vector<AZStd::pair<CameraViewMode, AZStd::string>> cameraOptionNames = {
                { CameraViewMode::FRONT, "Front" },   { CameraViewMode::BACK, "Back" }, { CameraViewMode::TOP, "Top" },
                { CameraViewMode::BOTTOM, "Bottom" }, { CameraViewMode::LEFT, "Left" }, { CameraViewMode::RIGHT, "Right" },
            };

            for (const auto& pair : cameraOptionNames)
            {
                CameraViewMode mode = pair.first;
                cameraMenu->addAction(
                    pair.second.c_str(),
                    [mode]()
                    {
                        // Send the reset camera event.
                        AnimViewportRequestBus::Broadcast(&AnimViewportRequestBus::Events::SetCameraViewMode, mode);
                    });
            }

            cameraMenu->addSeparator();
            cameraMenu->addAction(
                "Reset Camera",
                []()
                {
                    // Send the reset camera event.
                    AnimViewportRequestBus::Broadcast(&AnimViewportRequestBus::Events::ResetCamera);
                });

            cameraMenu->addSeparator();
            m_followCharacterAction = cameraMenu->addAction("Follow Character");
            m_followCharacterAction->setCheckable(true);
            m_followCharacterAction->setChecked(false);
            connect(m_followCharacterAction, &QAction::triggered, this,
                [this]()
                {
                    AnimViewportRequestBus::Broadcast(
                        &AnimViewportRequestBus::Events::SetFollowCharacter, m_followCharacterAction->isChecked());
                    ;
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
        SaveSettings();
    }

    void AnimViewportToolBar::CreateViewOptionEntry(
        QMenu* menu, const char* menuEntryName, uint32_t actionIndex, bool visible, char* iconFileName)
    {
        QAction* action = menu->addAction(
            menuEntryName,
            [actionIndex]()
            {
                // Send the reset camera event.
                AnimViewportRequestBus::Broadcast(
                    &AnimViewportRequestBus::Events::ToggleRenderFlag, (EMotionFX::ActorRenderFlag)actionIndex);
            });
        action->setCheckable(true);
        action->setVisible(visible);

        if (iconFileName)
        {
            action->setIcon(QIcon(iconFileName));
        }

        m_actions[actionIndex] = action;
    }

    void AnimViewportToolBar::SetRenderFlags(EMotionFX::ActorRenderFlagBitset renderFlags)
    {
        for (size_t i = 0; i < renderFlags.size(); ++i)
        {
            QAction* action = m_actions[i];
            if (action)
            {
                action->setChecked(renderFlags[i]);
            }
        }
    }

    void AnimViewportToolBar::LoadSettings()
    {
        AZStd::string renderFlagsFilename(EMStudioManager::GetInstance()->GetAppDataFolder());
        renderFlagsFilename += "AnimViewportRenderFlags.cfg";
        QSettings settings(renderFlagsFilename.c_str(), QSettings::IniFormat, this);

        const bool isChecked = settings.value("CameraFollowUp", false).toBool();
        m_followCharacterAction->setChecked(isChecked);
    }

    void AnimViewportToolBar::SaveSettings()
    {
        AZStd::string renderFlagsFilename(EMStudioManager::GetInstance()->GetAppDataFolder());
        renderFlagsFilename += "AnimViewportRenderFlags.cfg";
        QSettings settings(renderFlagsFilename.c_str(), QSettings::IniFormat, this);

        settings.setValue("CameraFollowUp", m_followCharacterAction->isChecked());
    }
} // namespace EMStudio

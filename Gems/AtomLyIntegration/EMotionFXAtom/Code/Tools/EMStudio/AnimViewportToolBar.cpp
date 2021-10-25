/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolButton>
#include <QMenu>
#include <EMStudio/AnimViewportToolBar.h>
#include <EMStudio/AnimViewportRequestBus.h>
#include <AzCore/std/string/string.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>


namespace EMStudio
{
    AnimViewportToolBar::AnimViewportToolBar(QWidget* parent)
        : QToolBar(parent)
    {
        AzQtComponents::ToolBar::addMainToolBarStyle(this);

        // Add the camera button
        QToolButton* cameraButton = new QToolButton(this);
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
        cameraMenu->addAction("Reset Camera",
            []()
            {
                // Send the reset camera event.
                AnimViewportRequestBus::Broadcast(&AnimViewportRequestBus::Events::ResetCamera);
            });
        cameraButton->setMenu(cameraMenu);
        cameraButton->setText("Camera Option");
        cameraButton->setPopupMode(QToolButton::InstantPopup);
        cameraButton->setVisible(true);
        cameraButton->setIcon(QIcon(":/EMotionFXAtom/Camera_category.svg"));
        addWidget(cameraButton);
    }
} // namespace EMStudio

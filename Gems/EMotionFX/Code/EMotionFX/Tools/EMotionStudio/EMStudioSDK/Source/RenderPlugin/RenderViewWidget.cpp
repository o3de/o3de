/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RenderViewWidget.h"
#include "RenderPlugin.h"
#include "../EMStudioCore.h"
#include "../PreferencesWindow.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <MysticQt/Source/KeyboardShortcutManager.h>

#include <QToolBar>


namespace EMStudio
{
    RenderViewWidget::RenderViewWidget(RenderPlugin* parentPlugin, QWidget* parentWidget)
        : QWidget(parentWidget)
    {
        for (uint32 i = 0; i < NUM_RENDER_OPTIONS; ++i)
        {
            m_toolbarButtons[i] = nullptr;
            m_actions[i] = nullptr;
        }

        m_renderOptionsWindow    = nullptr;
        m_plugin                 = parentPlugin;

        // create the vertical layout with the menu and the gl widget as entries
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(1);
        verticalLayout->setMargin(0);

        // create toolbar
        m_toolBar = new QToolBar(this);
        m_toolBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // add the toolbar to the vertical layout
        verticalLayout->addWidget(m_toolBar);

        QWidget* renderWidget = nullptr;
        m_plugin->CreateRenderWidget(this, &m_renderWidget, &renderWidget);
        verticalLayout->addWidget(renderWidget);

        new QActionGroup(this);

        QActionGroup* group = new QActionGroup(this);
        group->setExclusive(true);

        m_manipulatorModes[RenderOptions::SELECT] = AddToolBarAction("Select", "Select.svg");
        m_manipulatorModes[RenderOptions::TRANSLATE] = AddToolBarAction("Translate", "Translate.svg");
        m_manipulatorModes[RenderOptions::ROTATE] = AddToolBarAction("Rotate", "Rotate.svg");
        m_manipulatorModes[RenderOptions::SCALE] = AddToolBarAction("Scale", "Scale.svg");
        for (QAction* action : m_manipulatorModes)
        {
            action->setCheckable(true);
            group->addAction(action);
        }

        m_toolBar->addSeparator();

        QAction* layoutsAction = AddToolBarAction("Layouts", "Layout_category.svg");
        {
            QMenu* contextMenu = new QMenu(this);

            const AZStd::vector<RenderPlugin::Layout*>& layouts = m_plugin->GetLayouts();
            const RenderPlugin::Layout* currentLayout = m_plugin->GetCurrentLayout();
            for (RenderPlugin::Layout* layout : layouts)
            {
                QAction* layoutAction = contextMenu->addAction(layout->GetName());

                layoutAction->setCheckable(true);
                layoutAction->setChecked(layout == currentLayout);

                connect(layoutAction, &QAction::triggered, m_plugin, [this, layout](){
                    m_plugin->LayoutButtonPressed(layout->GetName());
                });
            }

            connect(layoutsAction, &QAction::toggled, contextMenu, &QMenu::show);
            layoutsAction->setMenu(contextMenu);

            auto widgetForAction = qobject_cast<QToolButton*>(m_toolBar->widgetForAction(layoutsAction));
            if (widgetForAction)
            {
                connect(layoutsAction, &QAction::triggered, widgetForAction, &QToolButton::showMenu);
            }
        }

        QAction* viewOptionsAction = AddToolBarAction("View options", "../Icons/Visualization.svg");
        {
            QMenu* contextMenu = new QMenu(this);

            CreateViewOptionEntry(contextMenu, "Solid", RENDER_SOLID);
            CreateViewOptionEntry(contextMenu, "Wireframe", RENDER_WIREFRAME);
            CreateViewOptionEntry(contextMenu, "Lighting", RENDER_LIGHTING);
            CreateViewOptionEntry(contextMenu, "Backface Culling", RENDER_BACKFACECULLING);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Vertex Normals", RENDER_VERTEXNORMALS);
            CreateViewOptionEntry(contextMenu, "Face Normals", RENDER_FACENORMALS);
            CreateViewOptionEntry(contextMenu, "Tangents", RENDER_TANGENTS);
            CreateViewOptionEntry(contextMenu, "Actor Bounding Boxes", RENDER_AABB);
            CreateViewOptionEntry(contextMenu, "Collision Meshes", RENDER_COLLISIONMESHES, false);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Line Skeleton", RENDER_LINESKELETON);
            CreateViewOptionEntry(contextMenu, "Solid Skeleton", RENDER_SKELETON);
            CreateViewOptionEntry(contextMenu, "Joint Names", RENDER_NODENAMES);
            CreateViewOptionEntry(contextMenu, "Joint Orientations", RENDER_NODEORIENTATION);
            CreateViewOptionEntry(contextMenu, "Actor Bind Pose", RENDER_ACTORBINDPOSE);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Hit Detection Colliders", RENDER_HITDETECTION_COLLIDERS, /*visible=*/true, ":/EMotionFX/HitDetection.svg");
            CreateViewOptionEntry(contextMenu, "Ragdoll Colliders", RENDER_RAGDOLL_COLLIDERS, /*visible=*/true, ":/EMotionFX/RagdollCollider.svg");
            CreateViewOptionEntry(contextMenu, "Ragdoll Joint Limits", RENDER_RAGDOLL_JOINTLIMITS, /*visible=*/true, ":/EMotionFX/RagdollJointLimit.svg");
            CreateViewOptionEntry(contextMenu, "Cloth Colliders", RENDER_CLOTH_COLLIDERS, /*visible=*/true, ":/EMotionFX/Cloth.svg");
            CreateViewOptionEntry(contextMenu, "Simulated Object Colliders", RENDER_SIMULATEDOBJECT_COLLIDERS, /*visible=*/true, ":/EMotionFX/SimulatedObjectCollider.svg");
            CreateViewOptionEntry(contextMenu, "Simulated Joints", RENDER_SIMULATEJOINTS, /*visible=*/true);

            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Motion Extraction", RENDER_MOTIONEXTRACTION);
            contextMenu->addSeparator();
            CreateViewOptionEntry(contextMenu, "Grid", RENDER_GRID);
            CreateViewOptionEntry(contextMenu, "Gradient Background", RENDER_USE_GRADIENTBACKGROUND);

            viewOptionsAction->setMenu(contextMenu);

            auto widgetForAction = qobject_cast<QToolButton*>(m_toolBar->widgetForAction(viewOptionsAction));
            if (widgetForAction)
            {
                connect(viewOptionsAction, &QAction::triggered, widgetForAction, &QToolButton::showMenu);
            }
        }

        QAction* cameraOptionsAction = AddToolBarAction("Camera options", "Camera_category.svg");
        {
            QMenu* cameraMenu = new QMenu(this);

            m_cameraModeActions.reserve(7);
            m_cameraModeActions.emplace_back(cameraMenu->addAction("Perspective", this, &RenderViewWidget::OnOrbitCamera), RenderWidget::CAMMODE_ORBIT);
            m_cameraModeActions.emplace_back(cameraMenu->addAction("Front", this, &RenderViewWidget::OnOrthoFrontCamera), RenderWidget::CAMMODE_FRONT);
            m_cameraModeActions.emplace_back(cameraMenu->addAction("Back", this, &RenderViewWidget::OnOrthoBackCamera), RenderWidget::CAMMODE_BACK);
            m_cameraModeActions.emplace_back(cameraMenu->addAction("Left", this, &RenderViewWidget::OnOrthoLeftCamera), RenderWidget::CAMMODE_LEFT);
            m_cameraModeActions.emplace_back(cameraMenu->addAction("Right", this, &RenderViewWidget::OnOrthoRightCamera), RenderWidget::CAMMODE_RIGHT);
            m_cameraModeActions.emplace_back(cameraMenu->addAction("Top", this, &RenderViewWidget::OnOrthoTopCamera), RenderWidget::CAMMODE_TOP);
            m_cameraModeActions.emplace_back(cameraMenu->addAction("Bottom", this, &RenderViewWidget::OnOrthoBottomCamera), RenderWidget::CAMMODE_BOTTOM);
            cameraMenu->addSeparator();

            cameraMenu->addAction("Reset Camera",      [this]() { this->OnResetCamera(); });

            QAction* showSelectedAction = cameraMenu->addAction("Show Selected", this, &RenderViewWidget::OnShowSelected);
            showSelectedAction->setShortcut(QKeySequence(Qt::Key_S + Qt::SHIFT));
            GetMainWindow()->GetShortcutManager()->RegisterKeyboardShortcut(showSelectedAction, RenderPlugin::s_renderWindowShortcutGroupName, true);
            addAction(showSelectedAction);

            QAction* showEntireSceneAction = cameraMenu->addAction("Show Entire Scene", this, &RenderViewWidget::OnShowEntireScene);
            showEntireSceneAction->setShortcut(QKeySequence(Qt::Key_A + Qt::SHIFT));
            GetMainWindow()->GetShortcutManager()->RegisterKeyboardShortcut(showEntireSceneAction, RenderPlugin::s_renderWindowShortcutGroupName, true);
            addAction(showEntireSceneAction);

            cameraMenu->addSeparator();

            m_followCharacterAction = cameraMenu->addAction(tr("Follow Character"));
            m_followCharacterAction->setCheckable(true);
            m_followCharacterAction->setChecked(true);
            connect(m_followCharacterAction, &QAction::triggered, this, &RenderViewWidget::OnFollowCharacter);

            cameraOptionsAction->setMenu(cameraMenu);
            
            m_cameraMenu = cameraMenu;

            auto widgetForAction = qobject_cast<QToolButton*>(m_toolBar->widgetForAction(cameraOptionsAction));
            if (widgetForAction)
            {
                connect(cameraOptionsAction, &QAction::triggered, widgetForAction, &QToolButton::showMenu);
            }
        }

        connect(m_manipulatorModes[RenderOptions::SELECT], &QAction::triggered, m_plugin, &RenderPlugin::SetSelectionMode);
        connect(m_manipulatorModes[RenderOptions::TRANSLATE], &QAction::triggered, m_plugin, &RenderPlugin::SetTranslationMode);
        connect(m_manipulatorModes[RenderOptions::ROTATE], &QAction::triggered, m_plugin, &RenderPlugin::SetRotationMode);
        connect(m_manipulatorModes[RenderOptions::SCALE], &QAction::triggered, m_plugin, &RenderPlugin::SetScaleMode);

        QAction* toggleSelectionBoxRendering = new QAction(
            "Toggle Selection Box Rendering",
            this
        );
        toggleSelectionBoxRendering->setShortcut(Qt::Key_J);
        GetMainWindow()->GetShortcutManager()->RegisterKeyboardShortcut(toggleSelectionBoxRendering, RenderPlugin::s_renderWindowShortcutGroupName, true);
        connect(toggleSelectionBoxRendering, &QAction::triggered, this, [this]
        {
            m_plugin->GetRenderOptions()->SetRenderSelectionBox(m_plugin->GetRenderOptions()->GetRenderSelectionBox() ^ true);
        });
        addAction(toggleSelectionBoxRendering);

        QAction* deleteSelectedActorInstance = new QAction(
            "Delete Selected Actor Instance",
            this
        );
        deleteSelectedActorInstance->setShortcut(Qt::Key_Delete);
        connect(deleteSelectedActorInstance, &QAction::triggered, []{ CommandSystem::RemoveSelectedActorInstances(); });
        addAction(deleteSelectedActorInstance);

        Reset();
        UpdateInterface();

        GetMainWindow()->LoadKeyboardShortcuts();
    }

    void RenderViewWidget::SetManipulatorMode(RenderOptions::ManipulatorMode mode)
    {
        m_manipulatorModes[mode]->setChecked(true);
    }

    void RenderViewWidget::Reset()
    {
        SetRenderFlag(RENDER_SOLID, true);
        SetRenderFlag(RENDER_WIREFRAME, false);

        SetRenderFlag(RENDER_LIGHTING, true);
        SetRenderFlag(RENDER_TEXTURING, false);
        SetRenderFlag(RENDER_SHADOWS, true);

        SetRenderFlag(RENDER_VERTEXNORMALS, false);
        SetRenderFlag(RENDER_FACENORMALS, false);
        SetRenderFlag(RENDER_TANGENTS, false);

        SetRenderFlag(RENDER_AABB, false);
        SetRenderFlag(RENDER_COLLISIONMESHES, false);
        SetRenderFlag(RENDER_RAGDOLL_COLLIDERS, true);
        SetRenderFlag(RENDER_RAGDOLL_JOINTLIMITS, true);
        SetRenderFlag(RENDER_HITDETECTION_COLLIDERS, true);
        SetRenderFlag(RENDER_CLOTH_COLLIDERS, true);
        SetRenderFlag(RENDER_SIMULATEDOBJECT_COLLIDERS, true);
        SetRenderFlag(RENDER_SIMULATEJOINTS, true);

        SetRenderFlag(RENDER_SKELETON, false);
        SetRenderFlag(RENDER_LINESKELETON, false);
        SetRenderFlag(RENDER_NODEORIENTATION, false);
        SetRenderFlag(RENDER_NODENAMES, false);
        SetRenderFlag(RENDER_ACTORBINDPOSE, false);
        SetRenderFlag(RENDER_MOTIONEXTRACTION, false);

        SetRenderFlag(RENDER_GRID, true);
        SetRenderFlag(RENDER_USE_GRADIENTBACKGROUND, true);
        SetRenderFlag(RENDER_BACKFACECULLING, false);
    }


    void RenderViewWidget::SetRenderFlag(ERenderFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;

        if (m_toolbarButtons[optionIndex])
        {
            m_toolbarButtons[optionIndex]->setChecked(isEnabled);
        }
        if (m_actions[optionIndex])
        {
            m_actions[optionIndex]->setChecked(isEnabled);
        }
    }

    void RenderViewWidget::CreateViewOptionEntry(QMenu* menu, const char* menuEntryName, int32 actionIndex, bool visible, const char* iconFilename)
    {
        QAction* action = menu->addAction(menuEntryName);
        action->setCheckable(true);
        action->setVisible(visible);

        if (iconFilename)
        {
            action->setIcon(QIcon(iconFilename));
        }

        if (actionIndex >= 0)
        {
            m_actions[actionIndex] = action;
        }
    }

    QAction* RenderViewWidget::AddToolBarAction(const char* entryName, const char* iconName)
    {
        AZStd::string iconFileName = "Images/Rendering/";
        iconFileName += iconName;
        const QIcon& icon = MysticQt::GetMysticQt()->FindIcon(iconFileName.c_str());

        QAction* action = m_toolBar->addAction(icon, entryName);

        return action;
    }


    // destructor
    RenderViewWidget::~RenderViewWidget()
    {
        m_plugin->RemoveViewWidget(this);
    }


    // show the global rendering options dialog
    void RenderViewWidget::OnOptions()
    {
        if (m_renderOptionsWindow == nullptr)
        {
            m_renderOptionsWindow = new PreferencesWindow(this);
            m_renderOptionsWindow->Init();

            AzToolsFramework::ReflectedPropertyEditor* generalPropertyWidget = m_renderOptionsWindow->FindPropertyWidgetByName("General");
            if (!generalPropertyWidget)
            {
                generalPropertyWidget = m_renderOptionsWindow->AddCategory("General");
                generalPropertyWidget->ClearInstances();
                generalPropertyWidget->InvalidateAll();
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                return;
            }

            PluginOptions* pluginOptions = m_plugin->GetOptions();
            AZ_Assert(pluginOptions, "Expected options in render plugin");
            generalPropertyWidget->AddInstance(pluginOptions, azrtti_typeid(pluginOptions));

            // Now add EMStudio settings
            generalPropertyWidget->SetAutoResizeLabels(true);
            generalPropertyWidget->Setup(serializeContext, nullptr, true);
            generalPropertyWidget->show();
            generalPropertyWidget->ExpandAll();
            generalPropertyWidget->InvalidateAll();
        }

        m_renderOptionsWindow->show();
    }


    void RenderViewWidget::OnShowSelected()
    {
        m_renderWidget->ViewCloseup(true, DEFAULT_FLIGHT_TIME);
    }


    void RenderViewWidget::OnShowEntireScene()
    {
        m_renderWidget->ViewCloseup(false, DEFAULT_FLIGHT_TIME);
    }


    void RenderViewWidget::SetCharacterFollowModeActive(bool active)
    {
        m_followCharacterAction->setChecked(active);
    }


    void RenderViewWidget::OnFollowCharacter()
    {
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* followInstance = selectionList.GetFirstActorInstance();

        if (followInstance && GetIsCharacterFollowModeActive() && m_renderWidget)
        {
            m_renderWidget->ViewCloseup(true, DEFAULT_FLIGHT_TIME, 1);
        }
    }

    void RenderViewWidget::UpdateInterface()
    {
        // Update the checkmarks for the camera mode.
        for (const auto& actionModePair : m_cameraModeActions)
        {
            QAction* action = actionModePair.first;
            action->setCheckable(true);
            action->setChecked(m_renderWidget->GetCameraMode() == actionModePair.second);
        }
    }

    void RenderViewWidget::SaveOptions(QSettings* settings)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            QString name = QString(i);
            settings->setValue(name, m_actions[i] ? m_actions[i]->isChecked() : false);
        }

        settings->setValue("CameraMode", (int32)m_renderWidget->GetCameraMode());
        settings->setValue("CharacterFollowMode", GetIsCharacterFollowModeActive());
    }


    void RenderViewWidget::LoadOptions(QSettings* settings)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            QString name = QString(i);
            const bool isEnabled = settings->value(name, m_actions[i] ? m_actions[i]->isChecked() : false).toBool();
            SetRenderFlag((ERenderFlag)i, isEnabled);
        }

        // Override some settings as we removed those from the menu.
        SetRenderFlag(RENDER_COLLISIONMESHES, false);
        SetRenderFlag(RENDER_TEXTURING, false);

        RenderWidget::CameraMode cameraMode = (RenderWidget::CameraMode)settings->value("CameraMode", (int32)m_renderWidget->GetCameraMode()).toInt();
        m_renderWidget->SwitchCamera(cameraMode);

        const bool followMode = settings->value("CharacterFollowMode", GetIsCharacterFollowModeActive()).toBool();
        SetCharacterFollowModeActive(followMode);

        UpdateInterface();
    }


    uint32 RenderViewWidget::FindActionIndex(QAction* action)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            if (m_actions[i] == action)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }

} // namespace EMStudio

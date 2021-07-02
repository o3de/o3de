/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            mToolbarButtons[i] = nullptr;
            mActions[i] = nullptr;
        }

        mRenderOptionsWindow    = nullptr;
        mPlugin                 = parentPlugin;

        // create the vertical layout with the menu and the gl widget as entries
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(1);
        verticalLayout->setMargin(0);

        // create toolbar
        mToolBar = new QToolBar(this);
        mToolBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // add the toolbar to the vertical layout
        verticalLayout->addWidget(mToolBar);

        QWidget* renderWidget = nullptr;
        mPlugin->CreateRenderWidget(this, &mRenderWidget, &renderWidget);
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

        mToolBar->addSeparator();

        QAction* layoutsAction = AddToolBarAction("Layouts", "Layout_category.svg");
        {
            QMenu* contextMenu = new QMenu(this);

            const AZStd::vector<RenderPlugin::Layout*>& layouts = mPlugin->GetLayouts();
            const RenderPlugin::Layout* currentLayout = mPlugin->GetCurrentLayout();
            for (RenderPlugin::Layout* layout : layouts)
            {
                QAction* layoutAction = contextMenu->addAction(layout->GetName());

                layoutAction->setCheckable(true);
                layoutAction->setChecked(layout == currentLayout);

                connect(layoutAction, &QAction::triggered, mPlugin, [this, layout](){
                    mPlugin->LayoutButtonPressed(layout->GetName());
                });
            }

            connect(layoutsAction, &QAction::toggled, contextMenu, &QMenu::show);
            layoutsAction->setMenu(contextMenu);

            auto widgetForAction = qobject_cast<QToolButton*>(mToolBar->widgetForAction(layoutsAction));
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
            CreateViewOptionEntry(contextMenu, "Joint OBBs", RENDER_OBB, false);
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

            auto widgetForAction = qobject_cast<QToolButton*>(mToolBar->widgetForAction(viewOptionsAction));
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
            showSelectedAction->setShortcut(Qt::Key_S);
            GetMainWindow()->GetShortcutManager()->RegisterKeyboardShortcut(showSelectedAction, RenderPlugin::s_renderWindowShortcutGroupName, true);
            addAction(showSelectedAction);

            QAction* showEntireSceneAction = cameraMenu->addAction("Show Entire Scene", this, &RenderViewWidget::OnShowEntireScene);
            showEntireSceneAction->setShortcut(Qt::Key_A);
            GetMainWindow()->GetShortcutManager()->RegisterKeyboardShortcut(showEntireSceneAction, RenderPlugin::s_renderWindowShortcutGroupName, true);
            addAction(showEntireSceneAction);

            cameraMenu->addSeparator();

            mFollowCharacterAction = cameraMenu->addAction(tr("Follow Character"));
            mFollowCharacterAction->setCheckable(true);
            mFollowCharacterAction->setChecked(true);
            connect(mFollowCharacterAction, &QAction::triggered, this, &RenderViewWidget::OnFollowCharacter);

            cameraOptionsAction->setMenu(cameraMenu);
            
            mCameraMenu = cameraMenu;

            auto widgetForAction = qobject_cast<QToolButton*>(mToolBar->widgetForAction(cameraOptionsAction));
            if (widgetForAction)
            {
                connect(cameraOptionsAction, &QAction::triggered, widgetForAction, &QToolButton::showMenu);
            }
        }

        connect(m_manipulatorModes[RenderOptions::SELECT], &QAction::triggered, mPlugin, &RenderPlugin::SetSelectionMode);
        connect(m_manipulatorModes[RenderOptions::TRANSLATE], &QAction::triggered, mPlugin, &RenderPlugin::SetTranslationMode);
        connect(m_manipulatorModes[RenderOptions::ROTATE], &QAction::triggered, mPlugin, &RenderPlugin::SetRotationMode);
        connect(m_manipulatorModes[RenderOptions::SCALE], &QAction::triggered, mPlugin, &RenderPlugin::SetScaleMode);

        QAction* toggleSelectionBoxRendering = new QAction(
            "Toggle Selection Box Rendering",
            this
        );
        toggleSelectionBoxRendering->setShortcut(Qt::Key_J);
        GetMainWindow()->GetShortcutManager()->RegisterKeyboardShortcut(toggleSelectionBoxRendering, RenderPlugin::s_renderWindowShortcutGroupName, true);
        connect(toggleSelectionBoxRendering, &QAction::triggered, this, [this]
        {
            mPlugin->GetRenderOptions()->SetRenderSelectionBox(mPlugin->GetRenderOptions()->GetRenderSelectionBox() ^ true);
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
        SetRenderFlag(RENDER_OBB, false);
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

        if (mToolbarButtons[optionIndex])
        {
            mToolbarButtons[optionIndex]->setChecked(isEnabled);
        }
        if (mActions[optionIndex])
        {
            mActions[optionIndex]->setChecked(isEnabled);
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
            mActions[actionIndex] = action;
        }
    }

    QAction* RenderViewWidget::AddToolBarAction(const char* entryName, const char* iconName)
    {
        AZStd::string iconFileName = "Images/Rendering/";
        iconFileName += iconName;
        const QIcon& icon = MysticQt::GetMysticQt()->FindIcon(iconFileName.c_str());

        QAction* action = mToolBar->addAction(icon, entryName);

        return action;
    }


    // destructor
    RenderViewWidget::~RenderViewWidget()
    {
        mPlugin->RemoveViewWidget(this);
    }


    // show the global rendering options dialog
    void RenderViewWidget::OnOptions()
    {
        if (mRenderOptionsWindow == nullptr)
        {
            mRenderOptionsWindow = new PreferencesWindow(this);
            mRenderOptionsWindow->Init();

            AzToolsFramework::ReflectedPropertyEditor* generalPropertyWidget = mRenderOptionsWindow->FindPropertyWidgetByName("General");
            if (!generalPropertyWidget)
            {
                generalPropertyWidget = mRenderOptionsWindow->AddCategory("General");
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

            PluginOptions* pluginOptions = mPlugin->GetOptions();
            AZ_Assert(pluginOptions, "Expected options in render plugin");
            generalPropertyWidget->AddInstance(pluginOptions, azrtti_typeid(pluginOptions));

            // Now add EMStudio settings
            generalPropertyWidget->SetAutoResizeLabels(true);
            generalPropertyWidget->Setup(serializeContext, nullptr, true);
            generalPropertyWidget->show();
            generalPropertyWidget->ExpandAll();
            generalPropertyWidget->InvalidateAll();
        }

        mRenderOptionsWindow->show();
    }


    void RenderViewWidget::OnShowSelected()
    {
        mRenderWidget->ViewCloseup(true, DEFAULT_FLIGHT_TIME);
    }


    void RenderViewWidget::OnShowEntireScene()
    {
        mRenderWidget->ViewCloseup(false, DEFAULT_FLIGHT_TIME);
    }


    void RenderViewWidget::SetCharacterFollowModeActive(bool active)
    {
        mFollowCharacterAction->setChecked(active);
    }


    void RenderViewWidget::OnFollowCharacter()
    {
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* followInstance = selectionList.GetFirstActorInstance();

        if (followInstance && GetIsCharacterFollowModeActive() && mRenderWidget)
        {
            mRenderWidget->ViewCloseup(true, DEFAULT_FLIGHT_TIME, 1);
        }
    }

    void RenderViewWidget::UpdateInterface()
    {
        // Update the checkmarks for the camera mode.
        for (const auto& actionModePair : m_cameraModeActions)
        {
            QAction* action = actionModePair.first;
            action->setCheckable(true);
            action->setChecked(mRenderWidget->GetCameraMode() == actionModePair.second);
        }
    }

    void RenderViewWidget::SaveOptions(QSettings* settings)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            QString name = QString(i);
            settings->setValue(name, mActions[i] ? mActions[i]->isChecked() : false);
        }

        settings->setValue("CameraMode", (int32)mRenderWidget->GetCameraMode());
        settings->setValue("CharacterFollowMode", GetIsCharacterFollowModeActive());
    }


    void RenderViewWidget::LoadOptions(QSettings* settings)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            QString name = QString(i);
            const bool isEnabled = settings->value(name, mActions[i] ? mActions[i]->isChecked() : false).toBool();
            SetRenderFlag((ERenderFlag)i, isEnabled);
        }

        // Override some settings as we removed those from the menu.
        SetRenderFlag(RENDER_OBB, false);
        SetRenderFlag(RENDER_COLLISIONMESHES, false);
        SetRenderFlag(RENDER_TEXTURING, false);

        RenderWidget::CameraMode cameraMode = (RenderWidget::CameraMode)settings->value("CameraMode", (int32)mRenderWidget->GetCameraMode()).toInt();
        mRenderWidget->SwitchCamera(cameraMode);

        const bool followMode = settings->value("CharacterFollowMode", GetIsCharacterFollowModeActive()).toBool();
        SetCharacterFollowModeActive(followMode);

        UpdateInterface();
    }


    uint32 RenderViewWidget::FindActionIndex(QAction* action)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            if (mActions[i] == action)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }

} // namespace EMStudio

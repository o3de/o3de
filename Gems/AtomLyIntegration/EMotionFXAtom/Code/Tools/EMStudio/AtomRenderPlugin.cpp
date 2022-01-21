/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/Viewport/ViewportManipulatorController.h> // TODO: move this
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

#include <EMStudio/AtomRenderPlugin.h>
#include <EMStudio/AnimViewportRenderer.h>
#include <EMStudio/AnimViewportToolBar.h>
#include <EMStudio/AnimViewportInputController.h>

#include <Integration/Components/ActorComponent.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <QHBoxLayout>
#include <QGuiApplication>

#pragma optimize("", off)
namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(AtomRenderPlugin, EMotionFX::EditorAllocator, 0);
    const AzToolsFramework::ManipulatorManagerId g_animManipulatorManagerId =
        AzToolsFramework::ManipulatorManagerId(AZ::Crc32("AnimManipulatorManagerId"));

    AtomRenderPlugin::AtomRenderPlugin()
        : DockWidgetPlugin()
        , m_translationManipulators(
              AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::Identity(), AZ::Vector3::CreateOne())
        , m_rotateManipulators(AZ::Transform::CreateIdentity())
        , m_scaleManipulators(AZ::Transform::CreateIdentity())
    {
    }

    AtomRenderPlugin::~AtomRenderPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(m_importActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeActorCallback, false);
        delete m_importActorCallback;
        delete m_removeActorCallback;
    }

    const char* AtomRenderPlugin::GetName() const
    {
        return "Atom Render Window (Preview)";
    }

    uint32 AtomRenderPlugin::GetClassID() const
    {
        return static_cast<uint32>(AtomRenderPlugin::CLASS_ID);
    }

    const char* AtomRenderPlugin::GetCreatorName() const
    {
        return "O3DE";
    }

    float AtomRenderPlugin::GetVersion() const
    {
        return 1.0f;
    }

    bool AtomRenderPlugin::GetIsClosable() const
    {
        return true;
    }

    bool AtomRenderPlugin::GetIsFloatable() const
    {
        return true;
    }

    bool AtomRenderPlugin::GetIsVertical() const
    {
        return false;
    }

    EMStudioPlugin* AtomRenderPlugin::Clone()
    {
        return new AtomRenderPlugin();
    }

    EMStudioPlugin::EPluginType AtomRenderPlugin::GetPluginType() const
    {
        return EMStudioPlugin::PLUGINTYPE_RENDERING;
    }

    QWidget* AtomRenderPlugin::GetInnerWidget()
    {
        return m_innerWidget;
    }

    void AtomRenderPlugin::ReinitRenderer()
    {
        m_animViewportWidget->Reinit();
        SetupManipulators();
    }

    bool AtomRenderPlugin::Init()
    {
        LoadRenderOptions();

        m_innerWidget = new QWidget();
        m_dock->setWidget(m_innerWidget);

        QVBoxLayout* verticalLayout = new QVBoxLayout(m_innerWidget);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(1);
        verticalLayout->setMargin(0);

        // Add the viewport widget
        m_animViewportWidget = new AnimViewportWidget(this);

        // Add the tool bar
        AnimViewportToolBar* toolBar = new AnimViewportToolBar(this, m_innerWidget);
        toolBar->SetRenderFlags(m_animViewportWidget->GetRenderFlags());

        verticalLayout->addWidget(toolBar);
        verticalLayout->addWidget(m_animViewportWidget);

        m_manipulatorManager = AZStd::make_shared<AzToolsFramework::ManipulatorManager>(g_animManipulatorManagerId);

        // Register command callbacks.
        m_importActorCallback = new ImportActorCallback(false);
        m_removeActorCallback = new RemoveActorCallback(false);
        EMStudioManager::GetInstance()->GetCommandManager()->RegisterCommandCallback("ImportActor", m_importActorCallback);
        EMStudioManager::GetInstance()->GetCommandManager()->RegisterCommandCallback("RemoveActor", m_removeActorCallback);

        return true;
    }

    void AtomRenderPlugin::SetupManipulators()
    {
        // Add the manipulator controller
        auto controller = AZStd::make_shared<AnimViewportInputController>();
        controller->m_manipulatorManager = m_manipulatorManager.get();
        m_animViewportWidget->GetControllerList()->Add(controller);

        // Gather information about the entity
        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        worldTransform.SetTranslation(m_animViewportWidget->GetAnimViewportRenderer()->GetCharacterCenter());
        AZ::EntityComponentIdPair idPair = m_animViewportWidget->GetAnimViewportRenderer()->GetEntityComponentIdPair();

        // Setup the translate manipulator
        m_translationManipulators.SetSpace(worldTransform);
        m_translationManipulators.AddEntityComponentIdPair(idPair);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_translationManipulators);
        m_translationManipulators.InstallLinearManipulatorMouseMoveCallback(
            [this, idPair](const AzToolsFramework::LinearManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalPosition(), idPair);
            });

        m_translationManipulators.InstallPlanarManipulatorMouseMoveCallback(
            [this, idPair](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalPosition(), idPair);
            });

        m_translationManipulators.InstallSurfaceManipulatorMouseMoveCallback(
            [this, idPair](const AzToolsFramework::SurfaceManipulator::Action& action)
            {
                OnManipulatorMoved(action.LocalPosition(), idPair);
            });

        // Setup the rotation manipulator
        m_rotateManipulators.SetSpace(worldTransform);
        m_rotateManipulators.AddEntityComponentIdPair(idPair);
        m_rotateManipulators.SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        m_rotateManipulators.ConfigureView(
            AzToolsFramework::RotationManipulatorRadius(), AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);
        m_rotateManipulators.InstallMouseMoveCallback(
            [this, idPair](const AzToolsFramework::AngularManipulator::Action& action)
            {
                OnManipulatorRotated(action.LocalOrientation(), idPair);
            });

        // Setup the scale manipulator
        m_scaleManipulators.SetSpace(worldTransform);
        m_scaleManipulators.AddEntityComponentIdPair(idPair);
        m_scaleManipulators.SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        m_scaleManipulators.ConfigureView(
            AzToolsFramework::LinearManipulatorAxisLength(), AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor, AzFramework::ViewportColors::ZAxisColor);
        m_scaleManipulators.InstallAxisMouseMoveCallback(
            [this, idPair](const AzToolsFramework::LinearManipulator::Action& action)
            {
                OnManipulatorScaled(action.LocalScale(), idPair);
            });
    }

    void AtomRenderPlugin::SetManipulatorMode(RenderOptions::ManipulatorMode mode)
    {
        if (!m_manipulatorManager)
        {
            return;
        }

        switch (mode)
        {
        case RenderOptions::ManipulatorMode::SELECT:
            // The AtomRenderPlugin doesn't implement a select mode
            break;
        case RenderOptions::ManipulatorMode::TRANSLATE:
            {
                m_translationManipulators.Register(g_animManipulatorManagerId);
                m_rotateManipulators.Unregister();
                m_scaleManipulators.Unregister();
            }
            break;
        case RenderOptions::ManipulatorMode::ROTATE:
            {
                m_translationManipulators.Unregister();
                m_rotateManipulators.Register(g_animManipulatorManagerId);
                m_scaleManipulators.Unregister();
            }
            break;
        case RenderOptions::ManipulatorMode::SCALE:
            {
                m_translationManipulators.Unregister();
                m_rotateManipulators.Unregister();
                m_scaleManipulators.Register(g_animManipulatorManagerId);
            }
            break;
        }
    }

    void AtomRenderPlugin::OnManipulatorMoved(const AZ::Vector3& position, [[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        m_translationManipulators.SetLocalPosition(position);
        AZ::TransformBus::Event(idPair.GetEntityId(), &AZ::TransformBus::Events::SetLocalTranslation, position);
    }

    void AtomRenderPlugin::OnManipulatorRotated(const AZ::Quaternion& rotation, [[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        AZ::TransformBus::Event(idPair.GetEntityId(), &AZ::TransformBus::Events::SetLocalRotationQuaternion, rotation);
    }

    void AtomRenderPlugin::OnManipulatorScaled(const AZ::Vector3& scale, [[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        AZ::TransformBus::Event(idPair.GetEntityId(), &AZ::TransformBus::Events::SetLocalUniformScale, scale.GetX());
    }

    void AtomRenderPlugin::LoadRenderOptions()
    {
        AZStd::string renderOptionsFilename(GetManager()->GetAppDataFolder());
        renderOptionsFilename += "EMStudioRenderOptions.cfg";
        QSettings settings(renderOptionsFilename.c_str(), QSettings::IniFormat, this);
        m_renderOptions = RenderOptions::Load(&settings);
    }

    const RenderOptions* AtomRenderPlugin::GetRenderOptions() const
    {
        return &m_renderOptions;
    }

    void AtomRenderPlugin::Render([[maybe_unused]]EMotionFX::ActorRenderFlagBitset renderFlags)
    {
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, m_animViewportWidget->GetViewportContext()->GetId());
        AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        namespace AztfVi = AzToolsFramework::ViewportInteraction;
        AztfVi::KeyboardModifiers keyboardModifiers;
        AztfVi::EditorModifierKeyRequestBus::BroadcastResult(
            keyboardModifiers, &AztfVi::EditorModifierKeyRequestBus::Events::QueryKeyboardModifiers);

        debugDisplay->DepthTestOff();
        m_manipulatorManager->DrawManipulators(
            *debugDisplay, m_animViewportWidget->GetCameraState(),
            BuildMouseInteractionInternal(
                AztfVi::MouseButtons(AztfVi::TranslateMouseButtons(QGuiApplication::mouseButtons())), keyboardModifiers,
                BuildMousePick(m_animViewportWidget->mapFromGlobal(QCursor::pos()))));
                // BuildMousePick(QPoint() /*WidgetToViewport(mapFromGlobal(QCursor::pos()))*/)));
        debugDisplay->DepthTestOn();
    }

    bool AtomRenderPlugin::InternalHandleMouseManipulatorInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent)
    {
        if (!m_manipulatorManager)
        {
            return false;
        }

        using AzToolsFramework::ViewportInteraction::MouseEvent;
        const auto& mouseInteraction = mouseInteractionEvent.m_mouseInteraction;

        // store the current interaction for use in DrawManipulators
        // m_currentInteraction = mouseInteraction;

        switch (mouseInteractionEvent.m_mouseEvent)
        {
        case MouseEvent::Down:
            return m_manipulatorManager->ConsumeViewportMousePress(mouseInteraction);
        case MouseEvent::DoubleClick:
            return false;
        case MouseEvent::Move:
            {
                const AzToolsFramework::ManipulatorManager::ConsumeMouseMoveResult mouseMoveResult =
                    m_manipulatorManager->ConsumeViewportMouseMove(mouseInteraction);
                return mouseMoveResult == AzToolsFramework::ManipulatorManager::ConsumeMouseMoveResult::Interacting;
            }
        case MouseEvent::Up:
            return m_manipulatorManager->ConsumeViewportMouseRelease(mouseInteraction);
        case MouseEvent::Wheel:
            return m_manipulatorManager->ConsumeViewportMouseWheel(mouseInteraction);
        default:
            return false;
        }
    }

    AzToolsFramework::ViewportInteraction::MousePick AtomRenderPlugin::BuildMousePick(const QPoint& point) const
    {
        AzToolsFramework::ViewportInteraction::MousePick mousePick;
        mousePick.m_screenCoordinates = AzToolsFramework::ViewportInteraction::ScreenPointFromQPoint(point);
        const auto [origin, direction] = m_animViewportWidget->ViewportScreenToWorldRay(mousePick.m_screenCoordinates);
        mousePick.m_rayOrigin = origin;
        mousePick.m_rayDirection = direction;
        return mousePick;
    }

    AzToolsFramework::ViewportInteraction::MouseInteraction AtomRenderPlugin::BuildMouseInteractionInternal(
        const AzToolsFramework::ViewportInteraction::MouseButtons buttons,
        const AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers,
        const AzToolsFramework::ViewportInteraction::MousePick& mousePick) const
    {
        AzToolsFramework::ViewportInteraction::MouseInteraction mouse;
        mouse.m_interactionId.m_cameraId = AZ::EntityId(AZ::EntityId::InvalidEntityId);
        mouse.m_interactionId.m_viewportId = m_animViewportWidget->GetViewportContext()->GetId();
        mouse.m_interactionId.m_entityContextId = m_animViewportWidget->GetAnimViewportRenderer()->GetEntityContextId();
        mouse.m_mouseButtons = buttons;
        mouse.m_mousePick = mousePick;
        mouse.m_keyboardModifiers = modifiers;
        return mouse;
    }

    // Command callbacks
    bool ReinitAtomRenderPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(static_cast<uint32>(AtomRenderPlugin::CLASS_ID));
        if (!plugin)
        {
            AZ_Error("AtomRenderPlugin", false, "Cannot execute command callback. Atom render plugin does not exist.");
            return false;
        }

        AtomRenderPlugin* atomRenderPlugin = static_cast<AtomRenderPlugin*>(plugin);
        atomRenderPlugin->ReinitRenderer();

        return true;
    }

    bool AtomRenderPlugin::ImportActorCallback::Execute(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }
    bool AtomRenderPlugin::ImportActorCallback::Undo(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }

    bool AtomRenderPlugin::RemoveActorCallback::Execute(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }
    bool AtomRenderPlugin::RemoveActorCallback::Undo(
        [[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReinitAtomRenderPlugin();
    }
}// namespace EMStudio

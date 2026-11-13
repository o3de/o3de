/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Maestro/Editor/MaestroEditorSystemComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Maestro/Editor/AnimationContext.h>
#include <Maestro/Editor/TrackView/TrackViewDialog.h>
#include <Maestro/Editor/TrackView/TrackViewSequenceManager.h>
#include <Maestro/IMovieSystem.h>

// Editor
#include <IEditor.h>
#include <QtViewPaneManager.h>

namespace Maestro
{
    MaestroEditorSystemComponent::MaestroEditorSystemComponent() = default;
    MaestroEditorSystemComponent::~MaestroEditorSystemComponent() = default;

    namespace Editor
    {
        namespace
        {
            CAnimationContext* s_animationContext = nullptr;
            CTrackViewSequenceManager* s_sequenceManager = nullptr;
        }

        CAnimationContext* GetAnimation()
        {
            return s_animationContext;
        }

        CTrackViewSequenceManager* GetSequenceManager()
        {
            return s_sequenceManager;
        }

        void SetAnimationContext(CAnimationContext* animationContext)
        {
            s_animationContext = animationContext;
        }

        void SetSequenceManager(CTrackViewSequenceManager* sequenceManager)
        {
            s_sequenceManager = sequenceManager;
        }

        void ReloadTrackView()
        {
            QWidget* viewPaneWidget = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                viewPaneWidget,
                &AzToolsFramework::EditorRequests::Bus::Events::GetViewPaneWidget,
                Editor::TrackViewPaneName);
            if (auto* trackViewDialog = qobject_cast<CTrackViewDialog*>(viewPaneWidget))
            {
                trackViewDialog->Reload();
            }
        }
    }

    void MaestroEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MaestroEditorSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void MaestroEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaestroEditorService"));
    }

    void MaestroEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaestroEditorService"));
    }

    void MaestroEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MaestroService"));
    }

    void MaestroEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        const AZ::InterfaceRegisterOutcome registerOutcome =
            AzToolsFramework::EditorSequenceSystemLifecycleInterface::Register(this);
        m_sequenceSystemLifecycleRegistered = registerOutcome.IsSuccess();
        AZ_Assert(m_sequenceSystemLifecycleRegistered, "An editor sequence system lifecycle provider is already registered.");

        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            editor, &AzToolsFramework::EditorRequests::Bus::Events::GetEditor);
        if (editor)
        {
            CreateEditorServices(editor);
        }
    }

    void MaestroEditorSystemComponent::Deactivate()
    {
        if (m_sequenceSystemLifecycleRegistered)
        {
            const AZ::InterfaceRegisterOutcome unregisterOutcome =
                AzToolsFramework::EditorSequenceSystemLifecycleInterface::Unregister(this);
            AZ_Assert(unregisterOutcome.IsSuccess(), "Failed to unregister the Maestro editor sequence system lifecycle provider.");
            m_sequenceSystemLifecycleRegistered = false;
        }

        if (m_viewRegistered)
        {
            if (QtViewPaneManager::exists())
            {
                QtViewPane* pane = QtViewPaneManager::instance()->GetPane(Editor::TrackViewPaneName);
                if (pane)
                {
                    // Finish removing widget listeners before destroying the services they use.
                    const QList<DockWidget*> dockWidgets = pane->m_dockWidgetInstances;
                    qDeleteAll(dockWidgets);
                }
            }
            AzToolsFramework::UnregisterViewPane(Editor::TrackViewPaneName);
            m_viewRegistered = false;
        }

        DestroyEditorServices();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void MaestroEditorSystemComponent::NotifyIEditorAvailable(IEditor* editor)
    {
        CreateEditorServices(editor);
    }

    void MaestroEditorSystemComponent::NotifyRegisterViews()
    {
        if (AZ::Interface<IMovieSystem>::Get() && !m_viewRegistered)
        {
            CTrackViewDialog::RegisterViewClass();
            m_viewRegistered = true;
        }
    }

    void MaestroEditorSystemComponent::NotifyEditorInitialized()
    {
        m_editorInitialized = true;
    }

    void MaestroEditorSystemComponent::OnEditorGameEngineInitialized(bool simulationMode)
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->EnablePhysicsEvents(simulationMode);
            m_gameEngineInitialized = true;
        }
    }

    void MaestroEditorSystemComponent::OnEditorGameEngineAttached()
    {
        InitializeAnimationContext();
    }

    void MaestroEditorSystemComponent::OnEditorGameEngineShutdown()
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->SetCallback(nullptr);
        }
        m_gameEngineInitialized = false;
    }

    void MaestroEditorSystemComponent::OnEditorUpdate()
    {
        if (m_animationContext)
        {
            m_animationContext->Update();
        }
    }

    void MaestroEditorSystemComponent::OnBeginSceneOpen()
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->RemoveAllSequences();
        }
    }

    void MaestroEditorSystemComponent::OnBeginGameMode()
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->EnablePhysicsEvents(true);
        }
    }

    void MaestroEditorSystemComponent::OnBeforePlayInEditorStart()
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->Reset(true, false);
        }
    }

    void MaestroEditorSystemComponent::OnAfterPlayInEditorStop()
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            for (int index = movieSystem->GetNumPlayingSequences(); --index >= 0;)
            {
                movieSystem->GetPlayingSequence(index)->Deactivate();
            }
            movieSystem->Reset(false, false);
        }
    }

    void MaestroEditorSystemComponent::OnEndGameMode(bool simulationMode)
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->EnablePhysicsEvents(simulationMode);
        }
    }

    void MaestroEditorSystemComponent::OnSimulationModeChanging(bool enabled)
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->EnablePhysicsEvents(enabled);
        }
    }

    void MaestroEditorSystemComponent::CreateEditorServices(IEditor* editor)
    {
        if (!editor || m_sequenceManager)
        {
            return;
        }

        m_editorInitialized = m_editorInitialized || editor->IsInitialized();
        m_sequenceManager = AZStd::make_unique<CTrackViewSequenceManager>();
        Editor::SetSequenceManager(m_sequenceManager.get());

        m_animationContext = AZStd::make_unique<CAnimationContext>();
        Editor::SetAnimationContext(m_animationContext.get());

        if (editor->GetGameEngine())
        {
            if (!m_gameEngineInitialized)
            {
                OnEditorGameEngineInitialized(gEnv && gEnv->IsEditorSimulationMode());
            }
            InitializeAnimationContext();
        }

        if (m_editorInitialized)
        {
            NotifyRegisterViews();
        }
    }

    void MaestroEditorSystemComponent::InitializeAnimationContext()
    {
        if (m_animationContext && !m_animationContextInitialized && AZ::Interface<IMovieSystem>::Get())
        {
            m_animationContext->Init();
            m_animationContextInitialized = true;
        }
    }

    void MaestroEditorSystemComponent::DestroyEditorServices()
    {
        if (IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get())
        {
            movieSystem->SetCallback(nullptr);
        }

        if (m_animationContext)
        {
            Editor::SetAnimationContext(nullptr);
            m_animationContext.reset();
        }

        if (m_sequenceManager)
        {
            m_sequenceManager.reset();
            Editor::SetSequenceManager(nullptr);
        }

        m_animationContextInitialized = false;
        m_editorInitialized = false;
        m_gameEngineInitialized = false;
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/EditorSequenceSystemLifecycle.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

class CAnimationContext;
class CTrackViewSequenceManager;
struct IEditor;

namespace Maestro
{
    class MaestroEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::IEditorSequenceSystemLifecycle
    {
    public:
        AZ_COMPONENT(MaestroEditorSystemComponent, "{5EEB71BB-9F53-45A5-B7D5-47874E6EAE79}");

        MaestroEditorSystemComponent();
        ~MaestroEditorSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        void Activate() override;
        void Deactivate() override;

    private:
        // AzToolsFramework::EditorEvents
        void NotifyIEditorAvailable(IEditor* editor) override;
        void NotifyRegisterViews() override;
        void NotifyEditorInitialized() override;

        // AzToolsFramework::IEditorSequenceSystemLifecycle
        void OnEditorGameEngineInitialized(bool simulationMode) override;
        void OnEditorGameEngineAttached() override;
        void OnEditorGameEngineShutdown() override;
        void OnEditorUpdate() override;
        void OnBeginSceneOpen() override;
        void OnBeginGameMode() override;
        void OnGameModeStarted() override;
        void OnGameModeStopped() override;
        void OnEndGameMode(bool simulationMode) override;
        void OnSimulationModeChanged(bool enabled) override;

        void CreateEditorServices(IEditor* editor);
        void InitializeAnimationContext();
        void DestroyEditorServices();

        IEditor* m_editor = nullptr;
        AZStd::unique_ptr<CTrackViewSequenceManager> m_sequenceManager;
        AZStd::unique_ptr<CAnimationContext> m_animationContext;
        bool m_editorInitialized = false;
        bool m_gameEngineInitialized = false;
        bool m_animationContextInitialized = false;
        bool m_viewRegistered = false;
        bool m_sequenceSystemLifecycleRegistered = false;
    };
}

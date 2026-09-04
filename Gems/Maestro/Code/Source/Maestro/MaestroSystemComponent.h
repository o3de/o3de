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

#include <AzFramework/API/SequenceSystemLifecycle.h>

#include <Maestro/Cinematics/Movie.h>
#include <Maestro/MaestroBus.h>

namespace Maestro
{
    class MaestroAllocatorComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MaestroAllocatorComponent, "{3636E0F4-5208-450F-83F4-BE09F6EE7FBC}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        MaestroAllocatorComponent() = default;
        ~MaestroAllocatorComponent() override = default;

        void Activate() override;
        void Deactivate() override;
    };

    //////////////////////////////////////////////////////////////////////////
    class MaestroSystemComponent
        : public AZ::Component
        , protected MaestroRequestBus::Handler
        , protected AzFramework::ISequenceSystemLifecycle
    {
    public:
        AZ_COMPONENT(MaestroSystemComponent, "{47991994-4417-4CD7-AE0B-FEF1C8720766}");

        MaestroSystemComponent();
        // The MaestroSystemComponent is a singleton, so should never by copied.
        MaestroSystemComponent(const MaestroSystemComponent&) = delete;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // MaestroRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        // AzFramework::ISequenceSystemLifecycle
        void OnSystemCVarRegistry() override;
        void OnSystemInitialized(bool skipSequenceSystem) override;
        void OnSystemShutdown() override;
        void OnPreUpdate(bool isEditorUpdate) override;
        void OnPostUpdate(bool isEditorUpdate) override;
        void OnLevelEntitiesReady() override;
        void OnLevelUnload() override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        void UpdateMovieSystem(bool preUpdate, bool isEditorUpdate);
        void ResetMovieSystem(bool playOnReset, bool removeSequences);
        void ShutdownMovieSystem();

        // singletons representing the movie system
        AZStd::unique_ptr<CMovieSystem> m_movieSystem;
        IConsole* m_console = nullptr;
        int m_trackViewEnabled = 1;
        float m_maxTimeStepForMovieSystem = 0.1f;
        bool m_sequenceSystemLifecycleRegistered = false;
    };
}

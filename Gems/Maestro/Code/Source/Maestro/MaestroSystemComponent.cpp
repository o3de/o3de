/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Maestro/MaestroSystemComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Translation/TranslationDef.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <IConsole.h>
#include <ISystem.h>

#include <Maestro/Cinematics/Movie.h>

namespace Maestro
{
    MaestroSystemComponent::MaestroSystemComponent() = default;

    void MaestroAllocatorComponent::Activate()
    {
    }

    void MaestroAllocatorComponent::Deactivate()
    {
    }

    void MaestroAllocatorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MemoryAllocators"));
    }

    void MaestroAllocatorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<MaestroAllocatorComponent, AZ::Component>()->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }));
        }
    }

    void MaestroSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaestroSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaestroSystemComponent>(
                    QT_TRANSLATE_NOOP("Maestro", "Maestro"),
                    QT_TRANSLATE_NOOP("Maestro", "Provides the Open 3D Engine Cinematics Service"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MaestroSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaestroService"));
    }

    void MaestroSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaestroService"));
    }

    void MaestroSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MemoryAllocators"));
    }

    void MaestroSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void MaestroSystemComponent::Init()
    {
    }

    void MaestroSystemComponent::Activate()
    {
        MaestroRequestBus::Handler::BusConnect();
        const AZ::InterfaceRegisterOutcome registerOutcome =
            AzFramework::SequenceSystemLifecycleInterface::Register(this);
        m_sequenceSystemLifecycleRegistered = registerOutcome.IsSuccess();
        AZ_Assert(m_sequenceSystemLifecycleRegistered, "A sequence system lifecycle provider is already registered.");
    }

    void MaestroSystemComponent::Deactivate()
    {
        if (m_sequenceSystemLifecycleRegistered)
        {
            const AZ::InterfaceRegisterOutcome unregisterOutcome =
                AzFramework::SequenceSystemLifecycleInterface::Unregister(this);
            AZ_Assert(unregisterOutcome.IsSuccess(), "Failed to unregister the Maestro sequence system lifecycle provider.");
            m_sequenceSystemLifecycleRegistered = false;
        }
        MaestroRequestBus::Handler::BusDisconnect();
        ShutdownMovieSystem();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void MaestroSystemComponent::OnSystemCVarRegistry()
    {
        if (m_console || !gEnv || !gEnv->pConsole)
        {
            return;
        }

        m_console = gEnv->pConsole;
        m_console->Register(
            "sys_maxTimeStepForMovieSystem",
            &m_maxTimeStepForMovieSystem,
            0.1f,
            VF_NULL,
            "Caps the time step for the movie system so that a cut-scene won't be jumped in the case of an extreme stall.");
        m_console->Register("sys_trackview", &m_trackViewEnabled, 1, VF_NULL, "Enables TrackView Update");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void MaestroSystemComponent::OnSystemInitialized(bool skipSequenceSystem)
    {
        m_movieSystem.reset();
        if (!skipSequenceSystem && gEnv && gEnv->pSystem)
        {
            m_movieSystem.reset(new CMovieSystem(gEnv->pSystem));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void MaestroSystemComponent::OnSystemShutdown()
    {
        ShutdownMovieSystem();
    }

    void MaestroSystemComponent::OnPreUpdate(bool isEditorUpdate)
    {
        UpdateMovieSystem(true, isEditorUpdate);
    }

    void MaestroSystemComponent::OnPostUpdate(bool isEditorUpdate)
    {
        UpdateMovieSystem(false, isEditorUpdate);
    }

    void MaestroSystemComponent::OnLevelEntitiesReady()
    {
        ResetMovieSystem(true, false);
    }

    void MaestroSystemComponent::OnLevelUnload()
    {
        ResetMovieSystem(false, true);
    }

    void MaestroSystemComponent::UpdateMovieSystem(bool preUpdate, bool isEditorUpdate)
    {
        if (!m_movieSystem || !m_trackViewEnabled || isEditorUpdate)
        {
            return;
        }

        float frameTime = AZ::TimeUsToSeconds(AZ::GetRealTickDeltaTimeUs());
        if (frameTime > m_maxTimeStepForMovieSystem)
        {
            frameTime = m_maxTimeStepForMovieSystem;
        }

        if (preUpdate)
        {
            m_movieSystem->PreUpdate(frameTime);
        }
        else
        {
            m_movieSystem->PostUpdate(frameTime);
        }
    }

    void MaestroSystemComponent::ResetMovieSystem(bool playOnReset, bool removeSequences)
    {
        if (!m_movieSystem)
        {
            return;
        }

        constexpr bool seekToStart = false;
        m_movieSystem->Reset(playOnReset, seekToStart);
        if (removeSequences)
        {
            m_movieSystem->RemoveAllSequences();
        }
    }

    void MaestroSystemComponent::ShutdownMovieSystem()
    {
        m_movieSystem.reset();

        if (m_console)
        {
            m_console->UnregisterVariable("sys_trackview");
            m_console->UnregisterVariable("sys_maxTimeStepForMovieSystem");
            m_console = nullptr;
        }
    }
}

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzManipulatorTestFramework/DirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace AzManipulatorTestFramework
{
    using MouseInteraction = AzToolsFramework::ViewportInteraction::MouseInteraction;
    using MouseInteractionEvent = AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    class CustomManipulatorManager : public AzToolsFramework::ManipulatorManager
    {
        using ManagerBase = AzToolsFramework::ManipulatorManager;

    public:
        using ManagerBase::ManagerBase;

        const AzToolsFramework::ManipulatorManagerId GetId() const;
        size_t GetRegisteredManipulatorCount() const;
    };

    //! Implementation of the manipulator interface using direct access to the manipulator manager.
    class DirectCallManipulatorManager : public ManipulatorManagerInterface
    {
    public:
        DirectCallManipulatorManager(
            ViewportInteractionInterface* viewportInteraction, AZStd::shared_ptr<CustomManipulatorManager> manipulatorManager);

        // ManipulatorManagerInterface ...
        void ConsumeMouseInteractionEvent(const MouseInteractionEvent& event) override;
        AzToolsFramework::ManipulatorManagerId GetId() const override;
        bool ManipulatorBeingInteracted() const override;

    private:
        // Trigger the updating of manipulator bounds.
        void DrawManipulators(const MouseInteraction& mouseInteraction);
        ViewportInteractionInterface* m_viewportInteraction;
        AZStd::shared_ptr<CustomManipulatorManager> m_manipulatorManager;
    };

    const AzToolsFramework::ManipulatorManagerId CustomManipulatorManager::GetId() const
    {
        return m_manipulatorManagerId;
    }

    size_t CustomManipulatorManager::GetRegisteredManipulatorCount() const
    {
        return m_manipulatorIdToPtrMap.size();
    }

    DirectCallManipulatorManager::DirectCallManipulatorManager(
        ViewportInteractionInterface* viewportInteraction, AZStd::shared_ptr<CustomManipulatorManager> manipulatorManager)
        : m_viewportInteraction(viewportInteraction)
        , m_manipulatorManager(AZStd::move(manipulatorManager))
    {
    }

    void DirectCallManipulatorManager::ConsumeMouseInteractionEvent(const MouseInteractionEvent& event)
    {
        using namespace AzToolsFramework::ViewportInteraction;
        const auto& mouseInteraction = event.m_mouseInteraction;

        DrawManipulators(mouseInteraction);

        switch (event.m_mouseEvent)
        {
        case MouseEvent::Down:
            {
                m_manipulatorManager->ConsumeViewportMousePress(mouseInteraction);
                break;
            }
        case MouseEvent::DoubleClick:
            {
                break;
            }
        case MouseEvent::Move:
            {
                m_manipulatorManager->ConsumeViewportMouseMove(mouseInteraction);
                break;
            }
        case MouseEvent::Up:
            {
                m_manipulatorManager->ConsumeViewportMouseRelease(mouseInteraction);
                break;
            }
        case MouseEvent::Wheel:
            {
                m_manipulatorManager->ConsumeViewportMouseWheel(mouseInteraction);
                break;
            }
        default:
            break;
        }

        DrawManipulators(mouseInteraction);
    }

    void DirectCallManipulatorManager::DrawManipulators(const MouseInteraction& mouseInteraction)
    {
        m_manipulatorManager->DrawManipulators(
            m_viewportInteraction->GetDebugDisplay(), m_viewportInteraction->GetCameraState(), mouseInteraction);
    }

    AzToolsFramework::ManipulatorManagerId DirectCallManipulatorManager::GetId() const
    {
        return m_manipulatorManager->GetId();
    }

    bool DirectCallManipulatorManager::ManipulatorBeingInteracted() const
    {
        return m_manipulatorManager->Interacting();
    }

    DirectCallManipulatorViewportInteraction::DirectCallManipulatorViewportInteraction(
        AZStd::shared_ptr<AzFramework::DebugDisplayRequests> debugDisplayRequests)
        : m_customManager(
              AZStd::make_unique<CustomManipulatorManager>(AzToolsFramework::ManipulatorManagerId(AZ::Crc32("TestManipulatorManagerId"))))
        , m_viewportInteraction(AZStd::make_unique<ViewportInteraction>(AZStd::move(debugDisplayRequests)))
        , m_manipulatorManager(AZStd::make_unique<DirectCallManipulatorManager>(m_viewportInteraction.get(), m_customManager))
    {
    }

    DirectCallManipulatorViewportInteraction::~DirectCallManipulatorViewportInteraction() = default;

    const ViewportInteractionInterface& DirectCallManipulatorViewportInteraction::GetViewportInteraction() const
    {
        return *m_viewportInteraction;
    }

    const ManipulatorManagerInterface& DirectCallManipulatorViewportInteraction::GetManipulatorManager() const
    {
        return *m_manipulatorManager;
    }
} // namespace AzManipulatorTestFramework

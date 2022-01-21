/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <MCore/Source/Command.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>

#include <EMStudio/AnimViewportWidget.h>
#include <EMStudio/ManipulatorInteractionRequestBus.h>
#include <QWidget>
#endif

namespace AZ
{
    class Entity;
}

namespace EMStudio
{
    class AtomRenderPlugin
        : public DockWidgetPlugin
        , private EMStudio::ManipulatorRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            CLASS_ID = 0x32b0c04d
        };
        AtomRenderPlugin();
        ~AtomRenderPlugin();

        // Plugin information
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;
        bool GetIsClosable() const override;
        bool GetIsFloatable() const override;
        bool GetIsVertical() const override;
        bool Init() override;
        EMStudioPlugin* Clone();
        EMStudioPlugin::EPluginType GetPluginType() const override;
        QWidget* GetInnerWidget();

        void ReinitRenderer();

        void LoadRenderOptions();
        void SaveRenderOptions();
        RenderOptions* GetRenderOptions();

        void Render(EMotionFX::ActorRenderFlagBitset renderFlags) override;
        void SetManipulatorMode(RenderOptions::ManipulatorMode mode);

    private:
        bool HandleMouseEvent(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent) override;

        // From a series of input primitives, compose a complete mouse interaction.
        AzToolsFramework::ViewportInteraction::MouseInteraction BuildMouseInteractionInternal(
            AzToolsFramework::ViewportInteraction::MouseButtons buttons,
            AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers,
            const AzToolsFramework::ViewportInteraction::MousePick& mousePick) const;
        AzToolsFramework::ViewportInteraction::MousePick BuildMousePick(const QPoint& point) const;

        void SetupManipulators();
        void OnManipulatorMoved(const AZ::Vector3& position);
        void OnManipulatorRotated(const AZ::Quaternion& rotation);
        void OnManipulatorScaled(const AZ::Vector3& scale, const AZ::Vector3& scaleOffset);

        QWidget* m_innerWidget = nullptr;
        AnimViewportWidget* m_animViewportWidget = nullptr;
        RenderOptions m_renderOptions;

        // Manipulators
        AzToolsFramework::TranslationManipulators m_translationManipulators;
        AzToolsFramework::RotationManipulators m_rotateManipulators;
        AzToolsFramework::ScaleManipulators m_scaleManipulators;
        AZStd::shared_ptr<AzToolsFramework::ManipulatorManager> m_manipulatorManager;

        MCORE_DEFINECOMMANDCALLBACK(ImportActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(RemoveActorCallback);
        ImportActorCallback* m_importActorCallback = nullptr;
        RemoveActorCallback* m_removeActorCallback = nullptr;
    };

    extern const AzToolsFramework::ManipulatorManagerId g_animManipulatorManagerId;
}// namespace EMStudio

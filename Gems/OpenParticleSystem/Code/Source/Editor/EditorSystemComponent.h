/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <Editor/ParticleBrowserInteractions.h>
#include <OpenParticleSystem/EditorParticleSystemComponentRequestBus.h>

namespace OpenParticle
{
    class EditorSystemComponent
        : public AZ::Component
        , public AzToolsFramework::EditorMenuNotificationBus::Handler
        , private EditorParticleSystemComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{891d3a87-cb36-4c5e-8ff7-43fd4b861ca2}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        AZ::TypeId GetParticleSystemConfigType() const override;

        AZ::TypeId GetEmitterConfigType() const override;

        AZStd::vector<AZ::TypeId> GetEmitTypes() const override;

        AZStd::vector<AZ::TypeId> GetSpawnTypes() const override;

        AZStd::vector<AZ::TypeId> GetUpdateTypes() const override;

        AZStd::vector<AZ::TypeId> GetRenderTypes() const override;

        AZ::TypeId GetDefaultEmitType() const override;

        AZStd::vector<AZ::TypeId> GetDefaultSpawnTypes() const override;

        AZ::TypeId GetDefaultRenderType() const override;

        EditorSystemComponent() = default;
        ~EditorSystemComponent() = default;

    protected:
        void Activate() override;
        void Deactivate() override;

        void OnResetToolMenuItems() override;

        //! EditorParticleSystemComponentRequestBus::Handler overrides...
        void OpenParticleEditor(const AZStd::string& sourcePath) override;

        void ResetMenu();

    private:
        QAction* m_openParticleEditorAction = nullptr;
        AZStd::unique_ptr<ParticleBrowserInteractions> m_particleBrowserInteractions;
    };
} // namespace OpenParticle

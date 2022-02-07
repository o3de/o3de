/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/functional.h>

/*!
 * \namespace LmbrCentral
 * LmbrCentral ties together systems from CryEngine and systems from the AZ framework.
 */
namespace LmbrCentral
{
    /*!
     * The LmbrCentral module class coordinates with the application
     * to reflect classes and create system components.
     *
     * Note that the \ref LmbrCentralEditorModule is used when working in the Editor.
     */
    AZ_PUSH_DISABLE_WARNING(4275, "-Wunknown-warning-option")
    class AZ_DLL_EXPORT LmbrCentralModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(LmbrCentralModule, "{7969B004-21A2-4D3D-AC8B-90A4FABCFF1E}", Module);

        LmbrCentralModule();
        ~LmbrCentralModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
    AZ_POP_DISABLE_WARNING

    /*!
     * The LmbrCentral system component performs initialization/shutdown tasks
     * in coordination with other system components.
     */
    class LmbrCentralSystemComponent
        : public AZ::Component
        , private AZ::Data::AssetManagerNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(LmbrCentralSystemComponent, "{CE249D37-C1D6-4A64-932D-C937B0EC2B8C}")
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        LmbrCentralSystemComponent() = default;
        ~LmbrCentralSystemComponent() override = default;

    private:
        LmbrCentralSystemComponent(const LmbrCentralSystemComponent&) = delete;
        ////////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////////

        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler> > m_assetHandlers;
        AZStd::vector<AZStd::unique_ptr<AZ::AssetTypeInfoBus::Handler> > m_unhandledAssetInfo;
        AZStd::vector<AZStd::function<void()>> m_allocatorShutdowns;
    };
} // namespace LmbrCentral

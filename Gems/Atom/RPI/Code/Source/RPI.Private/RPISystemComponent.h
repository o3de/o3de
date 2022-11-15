/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file RPISystemComponent.h
 * @brief Contains the definition of the RPISystemComponent that will actually have ownership of
 *        most RPI constructs and will be responsible for propagation to them as necessary
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Debug/PerformanceCollector.h>

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystem.h>
#include <Atom/RPI.Public/GpuQuery/GpuPassProfiler.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>

#include "PerformanceCVarManager.h"

namespace AZ
{
    namespace RPI
    {
        class MaterialFunctorSourceDataRegistration;

        /**
         * @brief The system level component of managing the RPI systems
         * @detail This class is mainly in charge of wrapping the RPISystem and
         *         providing access to other objects that live a the same execution level.
         *         This is the main entry point for adding GPU work to the RPI and for
         *         controlling RPI execution.
         */
        class RPISystemComponent final
            : public AZ::Component
            , public AZ::SystemTickBus::Handler
            , public AZ::RHI::RHISystemNotificationBus::Handler
            , public XRRegisterInterface::Registrar
            , public PerformanceCollectorOwner::Registrar
        {
        public:
            AZ_COMPONENT(RPISystemComponent, "{83E301F3-7A0C-4099-B530-9342B91B1BC0}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            RPISystemComponent();
            ~RPISystemComponent() override;

            void Activate() override;
            void Deactivate() override;

            ///////////////////////////////////////////////////////////////////
            // IXRRegisterInterface overrides
            void RegisterXRInterface(XRRenderingInterface* xrSystemInterface) override;
            void UnRegisterXRInterface() override;
            ///////////////////////////////////////////////////////////////////

        private:
            RPISystemComponent(const RPISystemComponent&) = delete;

            // SystemTickBus overrides...
            void OnSystemTick() override;
                        
            // RHISystemNotificationBus::Handler
            void OnDeviceRemoved(RHI::Device* device) override;

            ///////////////////////////////////////////////////////////////////
            // IPerformanceCollectorOwner overrides
            AZ::Debug::PerformanceCollector* GetPerformanceCollector() override { return m_performanceCollector.get(); }
            GpuPassProfiler* GetGpuPassProfiler() override { return m_gpuPassProfiler.get(); }
            ///////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////
            // Performance Collection

            //! Returns "Graphics-<OS>-<RHI>" string, which will be part of the output filename.
            //! It will make it easy to keep vulkan and dx12 results side by side.
            AZStd::string GetLogCategory();

            void InitializePerformanceCollector();

            static constexpr AZStd::string_view PerformanceLogCategory = "Graphics";
            static constexpr AZStd::string_view PerformanceSpecGraphicsSimulationTime = "Graphics Simulation Time";
            static constexpr AZStd::string_view PerformanceSpecGraphicsRenderTime = "Graphics Render Time";
            static constexpr AZStd::string_view PerformanceSpecEngineCpuTime = "Engine Cpu Time";
            static constexpr AZStd::string_view PerformanceSpecGpuTime = "Frame Gpu Time";

            AZStd::unique_ptr<AZ::Debug::PerformanceCollector> m_performanceCollector;
            AZStd::unique_ptr<GpuPassProfiler> m_gpuPassProfiler; // Used to measure "Render Pipeline Gpu Time".
            ///////////////////////////////////////////////////////////////////

            RPISystem m_rpiSystem;

            RPISystemDescriptor m_rpiDescriptor;

            MaterialFunctorSourceDataRegistration* m_materialFunctorRegistration = nullptr;
        };
    } // namespace RPI
} // namespace AZ

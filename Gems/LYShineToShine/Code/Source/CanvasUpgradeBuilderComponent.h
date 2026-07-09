/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace LYShineToShine
{
    //! Asset builder that detects old LyShine v1/v2 .uicanvas files and upgrades them
    //! to Shine v4 format in-place. This is a one-time migration builder:
    //!   - If the source .uicanvas is already v4, no job is emitted.
    //!   - If v1/v2, a job is emitted. ProcessJob converts the source file in-place
    //!     and produces no product asset (the Shine builder handles v4 → product).
    //!   - After AP detects the source change, the Shine builder processes the now-v4 file.
    //!
    //! Once all canvases are converted, remove the LYShineToShine gem entirely.
    class CanvasUpgradeBuilderComponent
        : public AZ::Component
        , public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_COMPONENT(CanvasUpgradeBuilderComponent, "{D4E8F1A2-B3C5-4D6E-9F7A-1B2C3D4E5F60}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AssetBuilderSDK::AssetBuilderCommandBus
        void ShutDown() override;

    private:
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response);

        bool m_isShuttingDown = false;
    };
} // namespace LYShineToShine

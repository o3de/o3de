/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialAsset;
        class MaterialTypeAsset;

        namespace MaterialBuilderUtils
        {
            //! @brief configure and register a job dependency with the job descriptor
            //! @param jobDescriptor job descriptor to which dependency will be added
            //! @param path path to the source file for the dependency
            //! @param jobKey job key for the builder processing the dependency
            //! @param platformId list of platform IDs to monitor for the job dependency
            //! @param subIds list of sub IDs that should be monitored for assets created by the job dependency
            //! @param updateFingerprint flag specifying if the job descriptor fingerprint should be updated with information from the
            //! dependency file
            //! @return reference to the new job dependency added to the job descriptor dependency container
            AssetBuilderSDK::JobDependency& AddJobDependency(
                AssetBuilderSDK::JobDescriptor& jobDescriptor,
                const AZStd::string& path,
                const AZStd::string& jobKey,
                const AZStd::string& platformId = {},
                const AZStd::vector<AZ::u32>& subIds = {},
                const bool updateFingerprint = true);

            //! Given a material asset that has been fully built and prepared,
            //! add any image dependencies as pre-load dependencies, to the job being emitted.
            //! This will cause them to auto preload as part of loading the material, as well as make sure
            //! they are included in any pak files shipped with the product.
            void AddImageAssetDependenciesToProduct(const AZ::RPI::MaterialAsset* materialAsset, AssetBuilderSDK::JobProduct& product);

            //! Same as the above overload, but for material TYPE assets.
            void AddImageAssetDependenciesToProduct(const AZ::RPI::MaterialTypeAsset* materialTypeAsset, AssetBuilderSDK::JobProduct& product);

            //! Measurement only: reports where a builder job's time actually went.
            //!
            //! The shader builder already has this. ExecuteShaderCompiler logs elapsedTimeMillis for every tool it runs, which is
            //! why a shader job's duration can be broken down into azslc, DXC and the rest, and why every optimisation so far has
            //! been aimed at that job. The material type and material jobs had no equivalent, so their durations in the Asset
            //! Processor's job list were opaque totals -- and between them they are roughly 40% of the time a Material Canvas edit
            //! takes to reach the viewport.
            //!
            //! Each Mark() closes the phase that was running and starts the next. The destructor closes the last one, so a job that
            //! returns early still reports what it managed to do.
            class JobPhaseTimer final
            {
            public:
                JobPhaseTimer(const char* builderName, AZStd::string_view jobName)
                    : m_builderName(builderName)
                    , m_jobName(jobName)
                    , m_start(AZStd::chrono::steady_clock::now())
                    , m_phaseStart(m_start)
                {
                }

                //! Closes the running phase under @phaseName. Call at each boundary, naming what just finished.
                void Mark(const char* phaseName)
                {
                    const auto now = AZStd::chrono::steady_clock::now();
                    m_phases.emplace_back(phaseName, AZStd::chrono::duration<double, AZStd::milli>(now - m_phaseStart).count());
                    m_phaseStart = now;
                }

                ~JobPhaseTimer()
                {
                    // Whatever ran after the last Mark, including teardown. Named rather than hidden so the phases always sum to
                    // the total and an unaccounted chunk is visible rather than quietly distributed.
                    Mark("(unmarked)");

                    const double totalMs =
                        AZStd::chrono::duration<double, AZStd::milli>(AZStd::chrono::steady_clock::now() - m_start).count();

                    AZ_TracePrintf(m_builderName, "Phase timings for %s:\n", m_jobName.c_str());
                    for (const auto& [phaseName, phaseMs] : m_phases)
                    {
                        AZ_TracePrintf(m_builderName, "    %-34s %6.0f ms\n", phaseName, phaseMs);
                    }
                    AZ_TracePrintf(m_builderName, "    %-34s %6.0f ms\n", "TOTAL", totalMs);
                }

                AZ_DISABLE_COPY_MOVE(JobPhaseTimer);

            private:
                const char* m_builderName;
                AZStd::string m_jobName;
                AZStd::chrono::steady_clock::time_point m_start;
                AZStd::chrono::steady_clock::time_point m_phaseStart;
                AZStd::vector<AZStd::pair<const char*, double>> m_phases;
            };

            //! Append a fingerprint value to the job descriptor using the file modification time of the specified file path
            void AddFingerprintForDependency(const AZStd::string& path, AssetBuilderSDK::JobDescriptor& jobDescriptor);
        } // namespace MaterialBuilderUtils
    } // namespace RPI
} // namespace AZ

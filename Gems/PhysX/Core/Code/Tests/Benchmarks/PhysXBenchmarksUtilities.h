/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <benchmark/benchmark.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/numeric.h>

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <Tests/PhysXTestCommon.h>

namespace AzPhysics
{
    class Scene;
    struct RigidBody;
}

namespace PhysX::Benchmarks
{
    namespace Types
    {
        //! Alias for the lists of frame and sub tick timing data.
        using TimeList = AZStd::vector<double>;

        //! Chrono duration to represent milliseconds as a double.
        using double_milliseconds = AZStd::chrono::duration<double, AZStd::chrono::milliseconds::period>;
    } // namespace Types

    namespace Utils
    {
        //! Function pointer to allow Shape configuration customization rigid bodies created with Utils::CreateRigidBodies. int param is the id of the rigid body being created (values 0-N, where N=number requested to be created)
        using GenerateColliderFuncPtr = AZStd::function<AZStd::shared_ptr<Physics::ShapeConfiguration>(int)>;
        //! Function pointer to allow spawn position customization rigid bodies created with Utils::CreateRigidBodies. int param is the id of the rigid body being created (values 0-N, where N=number requested to be created)
        using GenerateSpawnPositionFuncPtr = AZStd::function<const AZ::Vector3(int)>;
        //! Function pointer to allow spawn orientation customization rigid bodies created with Utils::CreateRigidBodies. int param is the id of the rigid body being created (values 0-N, where N=number requested to be created)
        using GenerateSpawnOrientationFuncPtr = AZStd::function<const AZ::Quaternion(int)>;
        //! Function pointer to allow setting the mass of the rigid bodies created with Utils::CreateRigidBodies. int param is the id of the rigid body being created (values 0-N, where N=number requested to be created)
        using GenerateMassFuncPtr = AZStd::function<float(int)>;
        //! Function pointer to allow setting an Entity Id on the rigid bodies created with Utils::CreateRigidBodies. int param is the id of the rigid body being created (values 0-N, where N=number requested to be created)
        using GenerateEntityIdFuncPtr = AZStd::function<AZ::EntityId(int)>;

        //! Type for returned objects when constructing rigid bodies. Depends on the desired type.
        using BenchmarkRigidBodies = AZStd::variant<AzPhysics::SimulatedBodyHandleList, PhysX::EntityList>;

        //! Helper function to create the required number of rigid bodies and spawn them in the provided world.
        //! @param numRigidBodies The number of bodies to spawn.
        //! @param sceneHandle The handle of a scene where the rigid bodies will be spawned into.
        //! @param enableCCD Flag to enable|disable Continuous Collision Detection (CCD).
        //! @param benchmarkObjectType Type specifying whether rigid bodies should be entities with components or API objects.
        //! @param genColliderFuncPtr [optional] Function pointer to allow caller to pick the collider object Default is a box sized at 1m.
        //! @param genSpawnPosFuncPtr [optional] Function pointer to allow caller to pick the spawn position.
        //! @param genSpawnOriFuncPtr [optional] Function pointer to allow caller to pick the spawn orientation.
        //! @param genMassFuncPtr [optional] Function pointer to allow caller to pick the mass of the object.
        //! @param genEntityIdFuncPtr [optional] Function pointer to allow caller to define the entity id of the object.
        BenchmarkRigidBodies CreateRigidBodies(
            int numRigidBodies,
            AzPhysics::SceneHandle sceneHandle,
            bool enableCCD,
            int benchmarkObjectType,
            GenerateColliderFuncPtr* genColliderFuncPtr = nullptr, GenerateSpawnPositionFuncPtr* genSpawnPosFuncPtr = nullptr,
            GenerateSpawnOrientationFuncPtr* genSpawnOriFuncPtr = nullptr, GenerateMassFuncPtr* genMassFuncPtr = nullptr,
            GenerateEntityIdFuncPtr* genEntityIdFuncPtr = nullptr,
            bool activateEntities = true
        );

        //! Helper that takes a list of SimulatedBodyHandles to Rigid Bodies and return RigidBody pointers
        AZStd::vector<AzPhysics::RigidBody*> GetRigidBodiesFromHandles(
            AzPhysics::Scene* scene, const Utils::BenchmarkRigidBodies& handlesList);

        //! Object that when given a World will listen to the Pre / Post physics updates.
        //! Will time the duration between Pre and Post events in milliseconds. Used for running Benchmarks
        struct PrePostSimulationEventHandler
        {
            PrePostSimulationEventHandler();

            //! Begin tracking the physics tick times.
            //! This will clear any previous recorded times
            //! @param world The physics world to track tick times
            void Start(AzPhysics::Scene* scene);

            //! Stop tracking the physics tick times.
            void Stop();

            Types::TimeList& GetSubTickTimes() { return m_subTickTimes; }

        private:
            void PreTick();
            void PostTick();
            //! list of each sub tick execution time in milliseconds
            Types::TimeList m_subTickTimes;
            AZStd::chrono::steady_clock::time_point m_tickStart;

            AzPhysics::SceneEvents::OnSceneSimulationStartHandler m_sceneStartSimHandler;
            AzPhysics::SceneEvents::OnSceneSimulationFinishHandler m_sceneFinishSimHandler;
        };

        //! This will calculate and return each requested percentiles of the data set provided.
        //! @param percentiles List of percentiles to return. values must be between 0.0 - 1.0.
        //! @param values Data set to find the percentile in. This will be modified by being partially sorted by the nth_element algorithm.
        //! @return Each percentile requested, ordered to match percentiles vector.
        template<class T>
        AZStd::vector<T> GetPercentiles(const AZStd::vector<double>& percentiles, AZStd::vector<T>& values)
        {
            AZStd::vector<T> results;
            if (values.empty() || percentiles.empty())
            {
                return results;
            }
            results.reserve(percentiles.size());
            for (double percentile : percentiles)
            {
                //ensure the percentile is between 0.0 and 1.0
                const double testEpsilon = 0.001;
                AZ::ClampIfCloseMag(percentile, 0.0, testEpsilon);
                AZ::ClampIfCloseMag(percentile, 1.0, testEpsilon);

                size_t idx = aznumeric_cast<size_t>(std::round(percentile * (values.size() - 1)));
                std::nth_element(values.begin(), values.begin() + idx, values.end());

                results.emplace_back(values[idx]);
            }

            return results;
        }

        //! Returned from GetStandardDeviationAndMean.
        //! Contains the calculated mean and standard deviation of the given data set.
        struct StandardDeviationAndMeanResults
        {
            double m_mean = 0.0;
            double m_standardDeviation = 0.0;
        };

        //! This will return standard deviation and mean of the data set provided.
        //! @param values Data set to use.
        //! @return A structure that contains the standard deviation and mean of the data set.
        template<class T>
        StandardDeviationAndMeanResults GetStandardDeviationAndMean(const AZStd::vector<T>& values)
        {
            StandardDeviationAndMeanResults res;
            if (values.empty())
            {
                return res;
            }

            //sum all values
            T sum = AZStd::accumulate(values.begin(), values.end(), T());
            //calculate mean
            res.m_mean = aznumeric_cast<double>(sum) / values.size();

            //calculate standard deviation
            auto calcStdev = [=](T sum, T calcStdev) -> T {
                const T deviation = calcStdev - aznumeric_cast<T>(res.m_mean);
                return sum + (deviation * deviation);
            };
            res.m_standardDeviation = aznumeric_cast<double>(AZStd::accumulate(values.begin(), values.end(), T(), calcStdev));
            res.m_standardDeviation = std::sqrt(res.m_standardDeviation / values.size());

            return res;
        }

        //! Helper to add frame and sub tick timing percentile stats to the benchmark.
        //! Adds the counters under the labels
        //! 'Frame-P{x}', 'Frame-Fastest', 'Frame-Slowest', 'SubTick-P{x}', 'SubTick-Fastest', 'SubTick-Slowest',
        //! where {x} is each requested percentile.
        //! @param state The benchmark::State object to add the counters.
        //! @param frameTimes List of execution times (milliseconds) of each game frame ran in the benchmark.
        //! @param subTickTimes List of execution times (milliseconds) of each physX sub tick ran in the benchmark.
        //! @param requestedPercentiles List of percentiles to calculate. Percentile values are 0.0 - 1.0. Defaults P50 (0.5), P90 (0.9), P99 (0.99).
        void ReportFramePercentileCounters(benchmark::State& state, Types::TimeList& frameTimes, Types::TimeList& subTickTimes, const AZStd::vector<double>& requestedPercentiles = { {0.5, 0.9, 0.99} });

        //! Helper function to add P50, P90, P99, fastest and slowest execution times from the provided list.
        //! Adds the counters under the labels 'P{x}', 'Fastest', 'Slowest',
        //! where {x} is each requested percentile.
        //! @param state The benchmark state to add the counters.
        //! @param executionTimes List of execution times to use to calculate the percentiles.
        //! @param requestedPercentiles List of percentiles to calculate. Percentile values are 0.0 - 1.0. Defaults P50 (0.5), P90 (0.9), P99 (0.99).
        template<typename T>
        void ReportPercentiles(benchmark::State& state, AZStd::vector<T>& executionTimes, const AZStd::vector<double>& requestedPercentiles = { {0.5, 0.9, 0.99} })
        {
            AZStd::vector<T> percentiles = GetPercentiles(requestedPercentiles, executionTimes);
            int minRange = static_cast<int>(AZStd::min(requestedPercentiles.size(), executionTimes.size()));
            for (int i = 0; i < minRange; i++)
            {
                AZStd::string label = AZStd::string::format("P%d", static_cast<int>(requestedPercentiles[i] * 100.0));
                state.counters[label.c_str()] = aznumeric_cast<double>(percentiles[i]);
            }

            std::nth_element(percentiles.begin(), percentiles.begin(), percentiles.end());
            state.counters["Fastest"] = !percentiles.empty() ? aznumeric_cast<double>(percentiles.front()) : -1.0;
            std::nth_element(percentiles.begin(), percentiles.begin() + (percentiles.size() - 1), percentiles.end());
            state.counters["Slowest"] = !percentiles.empty() ? aznumeric_cast<double>(percentiles.back()) : -1.0;
        }

        //! Helper to add frame and sub tick timing standard deviation and mean stats to the benchmark.
        //! Adds the counters under the labels 'Frame-StDev', 'Frame-Mean', 'SubTick-StDev', and 'SubTick-Mean'.
        //! @param state The benchmark::State object to add the counters.
        //! @param frameTimes List of execution times (milliseconds) of each game frame ran in the benchmark.
        //! @param subTickTimes List of execution times (milliseconds) of each physX sub tick ran in the benchmark.
        void ReportFrameStandardDeviationAndMeanCounters(benchmark::State& state, const Types::TimeList& frameTimes, const Types::TimeList& subTickTimes);

        //! Helper to add frame and sub tick timing standard deviation and mean stats to the benchmark
        //! Adds the counters under the labels 'StDev' and 'Mean'.
        //! @param state The benchmark::State object to add the counters.
        //! @param executionTimes List of execution times to use in the calculation.
        template<typename T>
        void ReportStandardDeviationAndMeanCounters(benchmark::State& state, const AZStd::vector<T>& executionTimes)
        {
            StandardDeviationAndMeanResults stdivMeanFrameTimes = GetStandardDeviationAndMean(executionTimes);
            state.counters["Mean"] = std::ceil(stdivMeanFrameTimes.m_mean * 1000.0) / 1000.0; //truncate to 3 decimal places
            state.counters["StDev"] = std::ceil(stdivMeanFrameTimes.m_standardDeviation * 1000.0) / 1000.0;
        }
    } // namespace Utils
} // namespace PhysX::Benchmarks

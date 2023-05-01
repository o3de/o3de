/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Benchmarks/PhysXBenchmarksUtilities.h>

#include <algorithm>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <Benchmarks/PhysXBenchmarksCommon.h>

namespace PhysX::Benchmarks
{
    namespace Utils
    {
        BenchmarkRigidBodies CreateRigidBodies(
            int numRigidBodies,
            AzPhysics::SceneHandle sceneHandle,
            bool enableCCD,
            int benchmarkObjectType,
            GenerateColliderFuncPtr* genColliderFuncPtr /*= nullptr*/, GenerateSpawnPositionFuncPtr* genSpawnPosFuncPtr /*= nullptr*/,
            GenerateSpawnOrientationFuncPtr* genSpawnOriFuncPtr /*= nullptr*/, GenerateMassFuncPtr* genMassFuncPtr /*= nullptr*/,
            GenerateEntityIdFuncPtr* genEntityIdFuncPtr /*= nullptr*/,
            bool activateEntities /*= true*/
        )
        {
            BenchmarkRigidBodies benchmarkRigidBodies;

            if (benchmarkObjectType == PhysX::Benchmarks::RigidBodyApiObject)
            {
                AzPhysics::SimulatedBodyHandleList rigidBodies;
                rigidBodies.reserve(numRigidBodies);
                benchmarkRigidBodies = AZStd::move(rigidBodies);
            }
            else if (benchmarkObjectType == PhysX::Benchmarks::RigidBodyEntity)
            {
                PhysX::EntityList rigidBodies;
                rigidBodies.reserve(numRigidBodies);
                benchmarkRigidBodies = AZStd::move(rigidBodies);
            }

            AzPhysics::RigidBodyConfiguration rigidBodyConfig;
            rigidBodyConfig.m_ccdEnabled = enableCCD;
            auto rigidBodyColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();

            auto defaultShapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne());
            for (int i = 0; i < numRigidBodies; i++)
            {
                //call the optional function pointers, otherwise assign a default
                if (genEntityIdFuncPtr != nullptr)
                {
                    rigidBodyConfig.m_entityId = (*genEntityIdFuncPtr)(i);
                }
                if (genMassFuncPtr != nullptr)
                {
                    rigidBodyConfig.m_mass = (*genMassFuncPtr)(i);
                }
                if (genSpawnPosFuncPtr != nullptr)
                {
                    rigidBodyConfig.m_position = (*genSpawnPosFuncPtr)(i);
                }
                if (genSpawnOriFuncPtr != nullptr)
                {
                    rigidBodyConfig.m_orientation = (*genSpawnOriFuncPtr)(i);
                }

                AZStd::shared_ptr<Physics::ShapeConfiguration> shapeConfig = nullptr;
                if (genColliderFuncPtr != nullptr)
                {
                    shapeConfig = (*genColliderFuncPtr)(i);
                }
                if (shapeConfig == nullptr)
                {
                    shapeConfig = defaultShapeConfiguration;
                }
                rigidBodyConfig.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(rigidBodyColliderConfig, shapeConfig);

                if (benchmarkObjectType == PhysX::Benchmarks::RigidBodyApiObject)
                {
                    AzPhysics::Scene* scene = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetScene(sceneHandle);
                    AzPhysics::SimulatedBodyHandle simBodyHandle = scene->AddSimulatedBody(&rigidBodyConfig);

                    AZStd::get_if<AzPhysics::SimulatedBodyHandleList>(&benchmarkRigidBodies)->push_back(simBodyHandle);
                }
                else if (benchmarkObjectType == PhysX::Benchmarks::RigidBodyEntity)
                {
                    EntityPtr entity;
                    if (rigidBodyConfig.m_entityId.IsValid())
                    {
                        entity = AZStd::make_shared<AZ::Entity>(rigidBodyConfig.m_entityId, "TestEntity");
                    }
                    else
                    {
                        entity = AZStd::make_shared<AZ::Entity>("TestEntity");
                    }

                    auto* transformComponent = entity->CreateComponent<AzFramework::TransformComponent>();
                    transformComponent->SetWorldTM(
                        AZ::Transform::CreateFromQuaternionAndTranslation(rigidBodyConfig.m_orientation, rigidBodyConfig.m_position));

                    auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
                    boxColliderComponent->SetShapeConfigurationList({ AzPhysics::ShapeColliderPair(rigidBodyColliderConfig, shapeConfig) });

                    entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig, sceneHandle);

                    entity->Init();
                    if (activateEntities)
                    {
                        entity->Activate();
                    }

                    AZStd::get_if<PhysX::EntityList>(&benchmarkRigidBodies)->push_back(entity);
                }
            }

            return benchmarkRigidBodies;
        }

        AZStd::vector<AzPhysics::RigidBody*> GetRigidBodiesFromHandles(
            AzPhysics::Scene* scene, const Utils::BenchmarkRigidBodies& benchmarkRigidBodies)
        {
            AZStd::vector<AzPhysics::RigidBody*> rigidBodies;

            if (auto handlesList = AZStd::get_if<AzPhysics::SimulatedBodyHandleList>(&benchmarkRigidBodies))
            {
                rigidBodies = AZStd::vector<AzPhysics::RigidBody*>(AZStd::from_range, *handlesList | AZStd::views::transform([&scene](const AzPhysics::SimulatedBodyHandle& handle) {
                    return azrtti_cast<AzPhysics::RigidBody*>(scene->GetSimulatedBodyFromHandle(handle)); }));

            }
            else if (auto entityList = AZStd::get_if<PhysX::EntityList>(&benchmarkRigidBodies))
            {
                rigidBodies = AZStd::vector<AzPhysics::RigidBody*>(AZStd::from_range, *entityList | AZStd::views::transform([](const EntityPtr& entity) {
                    return entity->FindComponent<RigidBodyComponent>()->GetRigidBody(); }));
            }

            return rigidBodies;
        }

        PrePostSimulationEventHandler::PrePostSimulationEventHandler()
            : m_sceneStartSimHandler([this](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltaTime)
                {
                    this->PreTick();
                })
            , m_sceneFinishSimHandler([this](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltatime)
                {
                    this->PostTick();
                })
        {

        }

        void PrePostSimulationEventHandler::Start(AzPhysics::Scene* scene)
        {
            m_subTickTimes.clear();
            scene->RegisterSceneSimulationStartHandler(m_sceneStartSimHandler);
            scene->RegisterSceneSimulationFinishHandler(m_sceneFinishSimHandler);
        }

        void PrePostSimulationEventHandler::Stop()
        {
            m_sceneStartSimHandler.Disconnect();
            m_sceneFinishSimHandler.Disconnect();
        }

        void PrePostSimulationEventHandler::PreTick()
        {
            m_tickStart = AZStd::chrono::steady_clock::now();
        }

        void PrePostSimulationEventHandler::PostTick()
        {
            auto tickElapsedMilliseconds = Types::double_milliseconds(AZStd::chrono::steady_clock::now() - m_tickStart);
            m_subTickTimes.emplace_back(tickElapsedMilliseconds.count());
        }

        void ReportFramePercentileCounters(benchmark::State& state, Types::TimeList& frameTimes, Types::TimeList& subTickTimes, const AZStd::vector<double>& requestedPercentiles /*= { {0.5, 0.9, 0.99} }*/)
        {
            AZStd::vector<double> framePercentiles = GetPercentiles(requestedPercentiles, frameTimes);
            AZStd::vector<double> subTickPercentiles = GetPercentiles(requestedPercentiles, subTickTimes);

            //Report the percentiles, slowest and fastest frame of the run
            int minRange = static_cast<int>(AZStd::min(requestedPercentiles.size(), framePercentiles.size()));
            for (int i = 0; i < minRange; i++)
            {
                AZStd::string label = AZStd::string::format("Frame-P%d", static_cast<int>(requestedPercentiles[i] * 100.0));
                state.counters[label.c_str()] = framePercentiles[i];
            }
            //add fastest and slowest frame time, if it doesn't exist report -1.0 (negative time is impossible, so this denotes an error).
            std::nth_element(frameTimes.begin(), frameTimes.begin(), frameTimes.end());
            state.counters["Frame-Fastest"] = !frameTimes.empty() ? frameTimes.front() : -1.0;
            std::nth_element(frameTimes.begin(), frameTimes.begin() + (frameTimes.size() - 1), frameTimes.end());
            state.counters["Frame-Slowest"] = !frameTimes.empty() ? frameTimes.back() : -1.0;

            //Report the percentiles, slowest and fastest sub tick of the run
            if (subTickTimes.empty())
            {
                return;
            }
            minRange = static_cast<int>(AZStd::min(requestedPercentiles.size(), subTickPercentiles.size()));
            for (int i = 0; i < minRange; i++)
            {
                AZStd::string label = AZStd::string::format("SubTick-P%d", static_cast<int>(requestedPercentiles[i] * 100.0));
                state.counters[label.c_str()] = subTickPercentiles[i];
            }
            //add fastest and slowest frame time, if it doesn't exist report -1.0 (negative time is impossible, so this denotes an error).
            std::nth_element(subTickTimes.begin(), subTickTimes.begin(), subTickTimes.end());
            state.counters["SubTick-Fastest"] = !subTickTimes.empty() ? subTickTimes.front() : -1.0;
            std::nth_element(subTickTimes.begin(), subTickTimes.begin() + (subTickTimes.size()-1), subTickTimes.end());
            state.counters["SubTick-Slowest"] = !subTickTimes.empty() ? subTickTimes.back() : -1.0;
        }

        void ReportFrameStandardDeviationAndMeanCounters(benchmark::State& state, const Types::TimeList& frameTimes, const Types::TimeList& subTickTimes)
        {
            StandardDeviationAndMeanResults stdivMeanFrameTimes = GetStandardDeviationAndMean(frameTimes);
            state.counters["Frame-Mean"] = aznumeric_cast<double>(aznumeric_cast<int64_t>(stdivMeanFrameTimes.m_mean * 1000.0)) / 1000.0; //truncate to 3 decimal places
            state.counters["Frame-StDev"] = aznumeric_cast<double>(aznumeric_cast<int64_t>(stdivMeanFrameTimes.m_standardDeviation * 1000.0)) / 1000.0;
            StandardDeviationAndMeanResults stdivMeanSubTickTimes = GetStandardDeviationAndMean(subTickTimes);
            state.counters["SubTick-Mean"] = aznumeric_cast<double>(aznumeric_cast<int64_t>(stdivMeanSubTickTimes.m_mean * 1000.0)) / 1000.0;
            state.counters["SubTick-StDev"] = aznumeric_cast<double>(aznumeric_cast<int64_t>(stdivMeanSubTickTimes.m_standardDeviation * 1000.0)) / 1000.0;
        }
    } // namespace Utils
} // namespace PhysX::Benchmarks

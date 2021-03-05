/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EMotionFX_precompiled.h"
#include <Integration/Components/AnimGraphNetSyncComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Serialize/MathMarshal.h>
#include <AzFramework/Network/NetBindingHandlerBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        namespace Network
        {
            /**
             * \brief This is a GridMate chunk that replicates Anim Graph parameters.
             * It's challenge is to replicate any of the supported parameter types where
             * the types are only known at runtime. To solve that, many datasets are created
             * with helper macros to avoid code duplication (@PARAM_DATASET and @PARAM_DATASET_NAME).
             *
             * For maximum compression, one should build a custom component that specifies the anim graph parameters by hand, for example:
             *
             * DataSet<float> m_param0;
             *
             * or if using delta compression feature of GridMate:
             *
             * DeltaCompressedDataSet<float, 1> m_param1;
             *
             * Active nodes (@m_activeNodes) change infrequently.
             *
             * Warning: @m_motionNodes motion nodes often do change frequently as their motion play time ticks down.
             * Care must be applied when aiming for the network budget of a project.
             */
            class AnimGraphNetSyncComponent::Chunk : public GridMate::ReplicaChunkBase
            {
            public:
                GM_CLASS_ALLOCATOR(Chunk);

                Chunk() : m_activeNodes("Active Nodes", NodeIndexContainer{}), m_motionNodes("Motion Nodes", MotionNodePlaytimeContainer{}) {}

                static const char* GetChunkName() { return "AnimGraphNetSyncComponent::Chunk"; }
                bool IsReplicaMigratable() override { return true; }

                using AnimDataSetType = GridMate::DataSet<AnimParameter, AnimParameterMarshaler, AnimParameterThrottler>;

                template <void (AnimGraphNetSyncComponent::* CallbackMethod)(const AnimParameter&, const GridMate::TimeContext&)>
                using AnimDataSet = AnimDataSetType::BindInterface<AnimGraphNetSyncComponent, CallbackMethod>;

                // A helper macro that creates a variable like this one:
                // AnimDataSet<&AnimGraphNetSyncComponent::OnAnimParameterChanged<0>> m_parameter0 = { "Param 0" };
    #define PARAM_DATASET( N ) AnimDataSet<&AnimGraphNetSyncComponent::OnAnimParameterChanged< N >> m_parameter##N = { "Param " #N }

                PARAM_DATASET(0);
                PARAM_DATASET(1);
                PARAM_DATASET(2);
                PARAM_DATASET(3);
                PARAM_DATASET(4);
                PARAM_DATASET(5);
                PARAM_DATASET(6);
                PARAM_DATASET(7);
                PARAM_DATASET(8);
                PARAM_DATASET(9);

                /*
                 * Note: GridMate by default supports up to 32 DataSets per ReplicaChunk: @GM_MAX_DATASETS_IN_CHUNK.
                 * That means that a component can sync up to 32 separate network fields. One can vary the number of supported number
                 * of parameters by simply creating new entries of @PARAM_DATASET above and @PARAM_DATASET_NAME below.
                 */

                // A collection of datasets that are used to synchronize anim graph parameters.
                AZStd::array<AnimDataSetType*, 10> m_parameters = { { // clang pre-6.0 requires double "{{" here but doesn't perform compile length verification :(
                    &m_parameter0,
                    &m_parameter1,
                    &m_parameter2,
                    &m_parameter3,
                    &m_parameter4,
                    &m_parameter5,
                    &m_parameter6,
                    &m_parameter7,
                    &m_parameter8,
                    &m_parameter9,
                } };

                GridMate::DataSet<NodeIndexContainer, NodeIndexContainerMarshaler>::
                    BindInterface<AnimGraphNetSyncComponent, &AnimGraphNetSyncComponent::OnActiveNodesChanged> m_activeNodes;
                GridMate::DataSet<MotionNodePlaytimeContainer, MotionNodePlaytimeContainerMarshaler>::
                    BindInterface<AnimGraphNetSyncComponent, &AnimGraphNetSyncComponent::OnMotionNodesChanged> m_motionNodes;
            };

            void AnimGraphNetSyncComponent::Reflect(AZ::ReflectContext* context)
            {
                GridMate::ReplicaChunkDescriptorTable& descTable = GridMate::ReplicaChunkDescriptorTable::Get();
                if (!descTable.FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(Chunk::GetChunkName())))
                {
                    descTable.RegisterChunkType<Chunk>();
                }

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AnimGraphNetSyncComponent, AZ::Component>()
                        ->Version(1)
                        ->Field( "Sync parameters", &AnimGraphNetSyncComponent::m_syncParameters )
                        ->Field( "Sync active nodes", &AnimGraphNetSyncComponent::m_syncActiveNodes )
                        ->Field( "Sync motion nodes", &AnimGraphNetSyncComponent::m_syncMotionNodes )
                        ;

                    AZ::EditContext* editContent = serializeContext->GetEditContext();
                    if (editContent)
                    {
                        editContent->Class<AnimGraphNetSyncComponent>("Anim Graph Net Sync",
                            "Replicates anim graph parameters over the network using GridMate")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::Category, "Networking")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/AnimGraphNetSync.svg")
                            ->DataElement( AZ::Edit::UIHandlers::Default, &AnimGraphNetSyncComponent::m_syncParameters, "Sync parameters",
                                "Synchronize parameters of the anim graph on the entity" )
                            ->DataElement( AZ::Edit::UIHandlers::Default, &AnimGraphNetSyncComponent::m_syncActiveNodes, "Sync active nodes",
                                "Synchronize active nodes in the anim graph on the entity" )
                            ->DataElement( AZ::Edit::UIHandlers::Default, &AnimGraphNetSyncComponent::m_syncMotionNodes, "Sync motion nodes",
                                "Synchronize motion nodes in the anim graph on the entity. Warning: this may take a significant amount of network bandwidth" )
                            ;
                    }
                }
            }

            void AnimGraphNetSyncComponent::Activate()
            {
                AnimGraphComponentNotificationBus::Handler::BusConnect(GetEntityId());
                
                if (m_syncMotionNodes || m_syncActiveNodes) // if there is anything synchronize over the network
                {
                    const bool isAuthoritative = AzFramework::NetQuery::IsEntityAuthoritative(GetEntityId());
                    if (isAuthoritative)
                    {
                        // Only the server (or authoritative entity) needs to watch the nodes values.
                        AZ::TickBus::Handler::BusConnect();
                    }

                    // We need to get anim graph instance. It will be either available to us now or later via a notification bus. See @OnAnimGraphInstanceCreated
                    AnimGraphComponentRequestBus::EventResult(m_instance, GetEntityId(), &AnimGraphComponentRequestBus::Events::GetAnimGraphInstance);
                    if (m_instance)
                    {
                        if (!m_instance->GetSnapshot())
                        {
                            m_instance->CreateSnapshot(isAuthoritative);
                        }
                    }
                }
            }

            void AnimGraphNetSyncComponent::Deactivate()
            {
                AnimGraphComponentNotificationBus::Handler::BusDisconnect();
                AZ::TickBus::Handler::BusDisconnect();
            }

            void AnimGraphNetSyncComponent::SetParameterOnClient(const AnimParameter& value, AZ::u8 index)
            {
                switch (value.m_type)
                {
                case AnimParameter::Type::Unsupported:
                    break;
                case AnimParameter::Type::Float:
                    AnimGraphComponentRequestBus::Event(GetEntityId(), &AnimGraphComponentRequestBus::Events::SetParameterFloat, index, value.m_value.f);
                    break;
                case AnimParameter::Type::Bool:
                    AnimGraphComponentRequestBus::Event(GetEntityId(), &AnimGraphComponentRequestBus::Events::SetParameterBool, index, value.m_value.b);
                    break;
                case AnimParameter::Type::Vector2:
                    AnimGraphComponentRequestBus::Event(GetEntityId(), &AnimGraphComponentRequestBus::Events::SetParameterVector2, index, value.m_value.v2);
                    break;
                case AnimParameter::Type::Vector3:
                    AnimGraphComponentRequestBus::Event(GetEntityId(), &AnimGraphComponentRequestBus::Events::SetParameterVector3, index, value.m_value.v3);
                    break;
                case AnimParameter::Type::Quaternion:
                    AnimGraphComponentRequestBus::Event(GetEntityId(), &AnimGraphComponentRequestBus::Events::SetParameterRotation, index, value.m_value.q);
                    break;
                default:
                    AZ_Assert(false, "Unsupported type");
                    break;
                }
            }

            template <AZ::u8 Index>
            void AnimGraphNetSyncComponent::OnAnimParameterChanged(const AnimParameter& value, const GridMate::TimeContext&)
            {
                SetParameterOnClient(value, Index);
            }

            template <AnimParameter::Type AnimParameterType, typename FieldType>
            void AnimGraphNetSyncComponent::SetParameterOnServer(AZ::u8 parameterIndex, const FieldType& newValue)
            {
                if (m_syncParameters)
                {
                    if (Chunk* chunk = GetChunk())
                    {
                        if (parameterIndex < chunk->m_parameters.size())
                        {
                            AnimParameter param;
                            param.m_type = AnimParameterType;

                            static_assert(sizeof(FieldType) <= sizeof(param.m_value), "The largest value param.m_value can store is a Quaternion");
                            // This is to simplify writing a value into a union.
                            // Ideally, one would use std::variant (C++17) instead of a union.
                            memcpy(&param.m_value, &newValue, sizeof(FieldType));

                            chunk->m_parameters[parameterIndex]->Set(param);
                        }
                        else
                        {
                            AZ_Warning("EMotionFX", false, "AnimGraphNetSyncComponent does not support synchronizing more than %u parameters", chunk->m_parameters.size());
                        }
                    }
                }
            }

            void AnimGraphNetSyncComponent::OnAnimGraphFloatParameterChanged(EMotionFX::AnimGraphInstance*,
                AZ::u32 parameterIndex,
                float beforeValue,
                float afterValue)
            {
                AZ_UNUSED(beforeValue);
                SetParameterOnServer<AnimParameter::Type::Float>(static_cast<AZ::u8>(parameterIndex), afterValue);
            }

            void AnimGraphNetSyncComponent::OnAnimGraphBoolParameterChanged(EMotionFX::AnimGraphInstance*,
                AZ::u32 parameterIndex,
                bool beforeValue,
                bool afterValue)
            {
                AZ_UNUSED(beforeValue);
                SetParameterOnServer<AnimParameter::Type::Bool>(static_cast<AZ::u8>(parameterIndex), afterValue);
            }

            void AnimGraphNetSyncComponent::OnAnimGraphStringParameterChanged(EMotionFX::AnimGraphInstance*,
                AZ::u32 parameterIndex,
                const char* beforeValue,
                const char* afterValue)
            {
                AZ_UNUSED(parameterIndex);
                AZ_UNUSED(beforeValue);
                AZ_UNUSED(afterValue);
                AZ_Warning("EMotionFX", false, "AnimGraphNetSync component does not supported synchronizing string parameters, please consider refactoring your anim graph to replace strings with integers or enum values.");
            }

            void AnimGraphNetSyncComponent::OnAnimGraphVector2ParameterChanged(EMotionFX::AnimGraphInstance*,
                AZ::u32 parameterIndex,
                const AZ::Vector2& beforeValue,
                const AZ::Vector2& afterValue)
            {
                AZ_UNUSED(beforeValue);
                SetParameterOnServer<AnimParameter::Type::Vector2>(static_cast<AZ::u8>(parameterIndex), afterValue);
            }

            void AnimGraphNetSyncComponent::OnAnimGraphVector3ParameterChanged(EMotionFX::AnimGraphInstance*,
                AZ::u32 parameterIndex,
                const AZ::Vector3& beforeValue,
                const AZ::Vector3& afterValue)
            {
                AZ_UNUSED(beforeValue);
                SetParameterOnServer<AnimParameter::Type::Vector3>(static_cast<AZ::u8>(parameterIndex), afterValue);
            }

            void AnimGraphNetSyncComponent::OnAnimGraphRotationParameterChanged(EMotionFX::AnimGraphInstance*,
                AZ::u32 parameterIndex,
                const AZ::Quaternion& beforeValue,
                const AZ::Quaternion& afterValue)
            {
                AZ_UNUSED(beforeValue);
                SetParameterOnServer<AnimParameter::Type::Quaternion>(static_cast<AZ::u8>(parameterIndex), afterValue);
            }

            void AnimGraphNetSyncComponent::OnActiveNodesChanged(const NodeIndexContainer& activeNodes, const GridMate::TimeContext& tc)
            {
                AZ_UNUSED(tc);
                // Client receiving values
                if (m_instance)
                {
                    if (const AZStd::shared_ptr<AnimGraphSnapshot> snapshot = m_instance->GetSnapshot())
                    {
                        snapshot->SetActiveNodes(activeNodes);
                    }
                }
            }

            void AnimGraphNetSyncComponent::OnMotionNodesChanged(const MotionNodePlaytimeContainer& motionNodes, const GridMate::TimeContext& tc)
            {
                AZ_UNUSED(tc);
                // Client receiving values
                if (m_instance)
                {
                    if (const AZStd::shared_ptr<AnimGraphSnapshot> snapshot = m_instance->GetSnapshot())
                    {
                        snapshot->SetMotionNodePlaytimes(motionNodes);
                    }
                }
            }

            bool AnimGraphNetSyncComponent::IsDifferent(const MotionNodePlaytimeContainer& oldList, const MotionNodePlaytimeContainer& newList) const
            {
                if (oldList.size() != newList.size())
                {
                    return true;
                }

                AZStd::size_t i = 0;
                for (auto& value : oldList)
                {
                    if (value.first != newList[i].first || value.second != newList[i].second)
                    {
                        return true;
                    }

                    ++i;
                }

                return false;
            }

            bool AnimGraphNetSyncComponent::IsDifferent(const NodeIndexContainer& oldList, const NodeIndexContainer& newList) const
            {
                if (oldList.size() != newList.size())
                {
                    return true;
                }

                AZStd::size_t i = 0;
                for (AZ::u32 value : oldList)
                {
                    if (value != newList[i])
                    {
                        return true;
                    }

                    ++i;
                }

                return false;
            }

            void AnimGraphNetSyncComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
            {
                AZ_UNUSED(deltaTime);
                AZ_UNUSED(time);

                if (!GetChunk())
                {
                    return; // network is not ready yet
                }

                if (m_instance)
                {
                    if (const AZStd::shared_ptr<AnimGraphSnapshot> snapshot = m_instance->GetSnapshot())
                    {
                        if (m_syncActiveNodes)
                        {
                            const NodeIndexContainer& activeNodes = snapshot->GetActiveNodes();
                            const NodeIndexContainer& currentValue = GetChunk()->m_activeNodes.Get();
                            if (IsDifferent(currentValue, activeNodes))
                            {
                                GetChunk()->m_activeNodes.Set(activeNodes); // Server sending the values
                            }
                        }

                        if (m_syncMotionNodes)
                        {
                            const MotionNodePlaytimeContainer& playTimes = snapshot->GetMotionNodePlaytimes();
                            const MotionNodePlaytimeContainer& currentTimes = GetChunk()->m_motionNodes.Get();
                            if (IsDifferent(currentTimes, playTimes))
                            {
                                GetChunk()->m_motionNodes.Set(playTimes); // Server sending the values
                            }
                        }
                    }
                }
            }

            void AnimGraphNetSyncComponent::OnAnimGraphInstanceCreated(EMotionFX::AnimGraphInstance* instance)
            {
                m_instance = instance;
                if (m_instance)
                {
                    const bool isAuthoritative = AzFramework::NetQuery::IsEntityAuthoritative(GetEntityId());
                    if (!m_instance->GetSnapshot())
                    {
                        m_instance->CreateSnapshot(isAuthoritative);
                    }
                }
            }

            void AnimGraphNetSyncComponent::OnAnimGraphInstanceDestroyed(EMotionFX::AnimGraphInstance*)
            {
                m_instance = nullptr;
            }

            AnimGraphNetSyncComponent::Chunk* AnimGraphNetSyncComponent::GetChunk() const
            {
                return static_cast<Chunk*>(m_chunk.get());
            }

            GridMate::ReplicaChunkPtr AnimGraphNetSyncComponent::GetNetworkBinding()
            {
                m_chunk = GridMate::CreateReplicaChunk<Chunk>();
                AZ_Assert(m_chunk, "Failed to create a chunk");

                if (m_instance)
                {
                    if (!m_instance->GetSnapshot())
                    {
                        m_instance->CreateSnapshot(true /* authoritative */);
                    }
                }

                return m_chunk;
            }

            void AnimGraphNetSyncComponent::SetNetworkBinding(GridMate::ReplicaChunkPtr chunk)
            {
                m_chunk = chunk;
                m_chunk->SetHandler(this);
            }

            void AnimGraphNetSyncComponent::UnbindFromNetwork()
            {
                AZ_Assert(m_chunk, "There wasn't any chunk present");
                if (m_chunk)
                {
                    m_chunk->SetHandler(nullptr);
                    m_chunk = nullptr;
                }
            }
        }
    } // namespace Integration
} // namespace EMotionFXAnimation

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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Network/NetBindable.h>
#include <Integration/AnimGraphComponentBus.h>
#include <Integration/Components/AnimGraphNetSyncTypes.h>

namespace EMotionFX
{
    namespace Integration
    {
        namespace Network
        {
            /**
             * \brief Generic solution for synchronizing parameters of Anim Graph component.
             * Synchronization is done over GridMate.
             *
             * Note that this is not the most optimal synchronization but it does
             * work for just about all Anim Graphs.
             *
             * Disclaimer: string parameters are not supported! Because one should not synchronize
             * strings over the network. They ought to be converted to enum/int values beforehand.
             */
            class AnimGraphNetSyncComponent
                : public AZ::Component
                , public AzFramework::NetBindable
                , public AnimGraphComponentNotificationBus::Handler
                , public AZ::TickBus::Handler
            {
            public:
                AZ_COMPONENT(AnimGraphNetSyncComponent, "{2F9428C1-0F07-4667-B052-40D9BC473AD3}", NetBindable);

                static void Reflect(AZ::ReflectContext* context);

                // AZ::Component interface implementation
                void Activate() override;
                void Deactivate() override;

                static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
                {
                    provided.push_back(AZ_CRC("EMotionFXAnimGraphNetSyncService", 0x42e6f127));
                }

                static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
                {
                    incompatible.push_back(AZ_CRC("EMotionFXAnimGraphNetSyncService", 0x42e6f127));
                }

                static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
                {
                    required.push_back(AZ_CRC("EMotionFXAnimGraphService", 0x9ec3c819));
                    required.push_back(AZ_CRC("ReplicaChunkService", 0xf86b88a8));
                }

            protected:
                // NetBindable interface implementation
                GridMate::ReplicaChunkPtr GetNetworkBinding() override;
                void SetNetworkBinding(GridMate::ReplicaChunkPtr chunk) override;
                void UnbindFromNetwork() override;

                // AnimGraphComponentNotificationBus interface implementation
                void OnAnimGraphFloatParameterChanged(EMotionFX::AnimGraphInstance*,
                    AZ::u32 parameterIndex,
                    float beforeValue,
                    float afterValue) override;
                void OnAnimGraphBoolParameterChanged(EMotionFX::AnimGraphInstance*,
                    AZ::u32 parameterIndex,
                    bool beforeValue,
                    bool afterValue) override;
                void OnAnimGraphStringParameterChanged(EMotionFX::AnimGraphInstance*,
                    AZ::u32 parameterIndex,
                    const char* beforeValue,
                    const char* afterValue) override;
                void OnAnimGraphVector2ParameterChanged(EMotionFX::AnimGraphInstance*,
                    AZ::u32 parameterIndex,
                    const AZ::Vector2& beforeValue,
                    const AZ::Vector2& afterValue) override;
                void OnAnimGraphVector3ParameterChanged(EMotionFX::AnimGraphInstance*,
                    AZ::u32 parameterIndex,
                    const AZ::Vector3& beforeValue,
                    const AZ::Vector3& afterValue) override;
                void OnAnimGraphRotationParameterChanged(EMotionFX::AnimGraphInstance*,
                    AZ::u32 parameterIndex,
                    const AZ::Quaternion& beforeValue,
                    const AZ::Quaternion& afterValue) override;

                // TickBus
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

                // AnimGraphComponentNotificationBus
                void OnAnimGraphInstanceCreated(EMotionFX::AnimGraphInstance* instance) override;
                void OnAnimGraphInstanceDestroyed(EMotionFX::AnimGraphInstance* instance) override;

            private:
                class Chunk;
                GridMate::ReplicaChunkPtr m_chunk;
                Chunk* GetChunk() const;

                // DataSet callback, it's a template to avoid duplicating similar callbacks
                template <AZ::u8 Index>
                void OnAnimParameterChanged(const AnimParameter& value, const GridMate::TimeContext& tc);

                // Helper on a client side
                void SetParameterOnClient(const AnimParameter& value, AZ::u8 index);

                // Helper on the server side to avoid duplicating very similar callbacks
                template <AnimParameter::Type AnimParameterType, typename FieldType>
                void SetParameterOnServer(AZ::u8 parameterIndex, const FieldType& newValue);

                /**
                 * \brief Optionally turn on or off replicating parameters of an anim graph on the same entity as this component.
                 */
                bool m_syncParameters = true;

                /**
                 * \brief Optionally turn on or off replicating active nodes of an anim graph on the same entity as this component.
                 */
                bool m_syncActiveNodes = false;
                /**
                 * \brief Optionally turn on or off replicating motion playtime nodes of an anim graph on the same entity as this component.
                 *
                 * It's off by default because these nodes are very frequently changing and would result in a high network bandwidth use.
                 */
                bool m_syncMotionNodes = false;

                // GridMate DataSet callback on clients
                void OnActiveNodesChanged(const NodeIndexContainer& activeNodes, const GridMate::TimeContext& tc);
                // GridMate DataSet callback on clients
                void OnMotionNodesChanged(const MotionNodePlaytimeContainer& motionNodes, const GridMate::TimeContext& tc);

                // Helper comparison method to avoid sending the same data
                bool IsDifferent(const NodeIndexContainer& oldList, const NodeIndexContainer& newList) const;
                // Helper comparison method to avoid sending the same data
                bool IsDifferent(const MotionNodePlaytimeContainer& oldList, const MotionNodePlaytimeContainer& newList) const;

                EMotionFX::AnimGraphInstance* m_instance = nullptr;
            };
        }
    } // namespace Integration
} // namespace EMotionFX

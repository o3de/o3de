/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/EditContext.h>
#include <ScriptCanvas/Core/Nodeable.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        template<typename t_Operand>
        float CalculateLerpBetweenNodeableDuration(t_Operand speed, t_Operand difference)
        {
            const auto speedLength = speed.GetLength();

            if (AZ::IsClose(float(speedLength), 0.0f, AZ::Constants::FloatEpsilon))
            {
                return -1.0f;
            }
            else
            {
                return fabsf(difference.GetLength() / speedLength);
            }
        }

        float CalculateLerpBetweenNodeableDuration(float speed, float difference);

        template<typename t_Operand>
        class LerpBetweenNodeable
            : public Nodeable
            , public AZ::SystemTickBus::Handler
            , public AZ::TickBus::Handler
        {
            using ThisType = LerpBetweenNodeable<t_Operand>;

        public:
            AZ_RTTI((LerpBetweenNodeable, "{3467EB2B-801E-4799-B47A-AFEA621A152B}", t_Operand), Nodeable);
            AZ_CLASS_ALLOCATOR(LerpBetweenNodeable<t_Operand>, AZ::SystemAllocator);

            static constexpr void DeprecatedTypeNameVisitor(
                const AZ::DeprecatedTypeNameCallback& visitCallback)
            {
                // LerpBetweenNodeable previously restricted the typename to 128 bytes
                AZStd::array<char, 128> deprecatedName{};

                // Due to the extra set of parenthesis, the actual type name of LerpBetweenNodeable started out literally as
                // "(LerpBetweenNodeable<t_Operand>)"
                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), "(LerpBetweenNodeable<t_Operand>)<");
                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), AZ::AzTypeInfo<t_Operand>::Name());
                // The old AZ::Internal::AggregateTypes implementations placed a space after each argument as a separator
                AZ::Internal::AzTypeInfoSafeCat(deprecatedName.data(), deprecatedName.size(), " >");

                if (visitCallback)
                {
                    visitCallback(deprecatedName.data());
                }
            }

            static void Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<LerpBetweenNodeable, Nodeable>()
                        ->Field("start", &LerpBetweenNodeable::m_start)
                        ->Field("difference", &LerpBetweenNodeable::m_difference)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<LerpBetweenNodeable>("Lerp Between", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ;
                    }
                }

                if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
                {
                    behaviorContext->Class<ThisType>()
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                        ->Method("In", &ThisType::In)
                        ->Method("Cancel", &ThisType::Cancel)
                        ;
                }
            }

            void Cancel()
            {
                StopLerp();
            }

            void OnDeactivate() override
            {
                StopLerp();
            }

            void In(t_Operand start, t_Operand stop, t_Operand rate, float maxDuration)
            {
                StopLerp();

                AZ::SystemTickBus::Handler::BusConnect();
                m_durationCurrent = 0;
                m_start = start;
                m_difference = stop - start;

                const float speedOnlyTime = CalculateLerpBetweenNodeableDuration(rate, m_difference);

                if (speedOnlyTime >= 0.0f)
                {
                    if (maxDuration >= 0.0f)
                    {
                        m_durationMax = AZStd::min(speedOnlyTime, maxDuration);
                    }
                    else
                    {
                        m_durationMax = speedOnlyTime;
                    }
                }
                else if (maxDuration >= 0.0f)
                {
                    m_durationMax = maxDuration;
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "Lerp Between not given a valid speed or duration to for the interpolation. Using 1 second duration.");
                    m_durationMax = 1.0f;
                }

                if (AZ::IsClose(m_durationMax, 0.0f, AZ::Constants::FloatEpsilon))
                {
                    StopLerp();
                    Lerp(1.0f);
                }
            }

            bool IsActive() const override
            {
                return AZ::TickBus::Handler::BusIsConnected()
                    || AZ::SystemTickBus::Handler::BusIsConnected();
            }

        protected:
            size_t GetRequiredOutCount() const override
            {
                return 2;
            }

            void Lerp(float t)
            {
                const t_Operand step = m_start + (m_difference * t);
                // make a release note that the lerp complete and tick slot are two different execution threads
                ExecutionOut(0, step, t);

                if (AZ::IsClose(t, 1.0f, AZ::Constants::FloatEpsilon))
                {
                    StopLerp();
                    ExecutionOut(1);
                }
            }

            // SystemTickBus
            void OnSystemTick() override
            {
                // This switch between the system and tick bus provides consistent starting point for the lerp.
                // It will always be on the next tick loop, no matter where this entity is in the current one.
                AZ::SystemTickBus::Handler::BusDisconnect();
                AZ::TickBus::Handler::BusConnect();

            }
            ////

            // TickBus
            void OnTick(float deltaTime, AZ::ScriptTimePoint) override
            {
                m_durationCurrent += deltaTime;
                m_durationCurrent = AZStd::min(m_durationCurrent, m_durationMax);
                Lerp(m_durationCurrent / m_durationMax);
            }
            ////

            void StopLerp()
            {
                AZ::SystemTickBus::Handler::BusDisconnect();
                AZ::TickBus::Handler::BusDisconnect();
            }

        private:
            t_Operand m_start;
            t_Operand m_difference;

            float m_durationCurrent;
            float m_durationMax;
        };

    }
}

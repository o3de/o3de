/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Color.h>

#include <ScriptedEntityTweener/ScriptedEntityTweenerBus.h>

#include "ScriptedEntityTweenerSubtask.h"

AZ_PUSH_DISABLE_WARNING(4244, "-Wunknown-warning-option")
#include "ScriptedEntityTweenerMath.h"
AZ_POP_DISABLE_WARNING

namespace ScriptedEntityTweener
{
    namespace SubtaskHelper
    {
        template <typename T>
        AZ_INLINE void DoSafeSet(AZ::BehaviorEBus::VirtualProperty* prop, AZ::EntityId entityId, T& data)
        {
            if (!prop || !prop->m_setter)
            {
                return;
            }
            if (prop->m_setter->m_event)
            {
                prop->m_setter->m_event->Invoke(entityId, data);
            }
            else if (prop->m_setter->m_broadcast)
            {
                prop->m_setter->m_broadcast->Invoke(data);
            }
        }

        template <typename T>
        AZ_INLINE void DoSafeGet(AZ::BehaviorEBus::VirtualProperty* prop, AZ::EntityId entityId, T& data)
        {
            if (!prop || !prop->m_getter)
            {
                return;
            }
            if (prop->m_getter->m_event)
            {
                prop->m_getter->m_event->InvokeResult(data, entityId);
            }
            else if (prop->m_getter->m_broadcast)
            {
                prop->m_getter->m_broadcast->InvokeResult(data);
            }
        }
    }

    bool ScriptedEntityTweenerSubtask::Initialize(const AnimationParameterAddressData& animParamData, const AZStd::any& targetValue, const AnimationProperties& properties)
    {
        Reset();
        if (CacheVirtualProperty(animParamData))
        {
            // Set initial value
            if (GetVirtualValue(m_valueInitial))
            {
                if (GetValueFromAny(m_valueTarget, targetValue))
                {
                    m_isActive = true;
                    m_animationProperties = properties;

                    if (m_animationProperties.m_isFrom)
                    {
                        EntityAnimatedValue initialValue = m_valueInitial;
                        m_valueInitial = m_valueTarget;
                        m_valueTarget = initialValue;
                    }
                    return true;
                }
            }
        }
        AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::Initialize - Initialization failed for [%s, %s]", m_animParamData.m_componentName.c_str(), m_animParamData.m_virtualPropertyName.c_str());
        return false;
    }

    void ScriptedEntityTweenerSubtask::Update(float deltaTime, AZStd::set<CallbackData>& callbacks)
    {
        if (m_isPaused || !m_isActive)
        {
            return;
        }
        //TODO: Use m_animationProperties.m_amplitudeOverride
        
        float timeAnimationActive = AZ::GetClamp(m_timeSinceStart + m_animationProperties.m_timeIntoAnimation, .0f, m_animationProperties.m_timeDuration);

        //If animation is meant to complete instantly, set timeToComplete and timeAnimationActive to the same non-zero value, so GetEasingResult will return m_valueTarget
        if (m_animationProperties.m_timeDuration == 0)
        {
            m_animationProperties.m_timeDuration = timeAnimationActive = 1.0f;
        }

        EntityAnimatedValue currentValue;
        if (m_virtualPropertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float initialValue;
            m_valueInitial.GetValue(initialValue);

            float targetValue;
            m_valueTarget.GetValue(targetValue);

            currentValue.SetValue(EasingEquations::GetEasingResult(m_animationProperties.m_easeMethod, m_animationProperties.m_easeType, timeAnimationActive, m_animationProperties.m_timeDuration, initialValue, targetValue));

            SetVirtualValue(currentValue);
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid() || m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid())
        {
            AZ::Vector3 initialValue;
            m_valueInitial.GetValue(initialValue);

            AZ::Vector3 targetValue;
            m_valueTarget.GetValue(targetValue);

            currentValue.SetValue(EasingEquations::GetEasingResult(m_animationProperties.m_easeMethod, m_animationProperties.m_easeType, timeAnimationActive, m_animationProperties.m_timeDuration, initialValue, targetValue));

            SetVirtualValue(currentValue);
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid())
        {
            AZ::Quaternion initialValue;
            m_valueInitial.GetValue(initialValue);

            AZ::Quaternion targetValue;
            m_valueTarget.GetValue(targetValue);

            currentValue.SetValue(EasingEquations::GetEasingResult(m_animationProperties.m_easeMethod, m_animationProperties.m_easeType, timeAnimationActive, m_animationProperties.m_timeDuration, initialValue, targetValue));

            SetVirtualValue(currentValue);
        }

        float progressPercent = timeAnimationActive / m_animationProperties.m_timeDuration;
        if (m_animationProperties.m_isPlayingBackward)
        {
            progressPercent = 1.0f - progressPercent;
        }

        if (progressPercent >= 1.0f)
        {
            m_timesPlayed = m_timesPlayed + 1;
            if (m_timesPlayed >= m_animationProperties.m_timesToPlay && m_animationProperties.m_timesToPlay != -1)
            {
                m_isActive = false;
                if (m_animationProperties.m_onCompleteCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::OnComplete, m_animationProperties.m_onCompleteCallbackId));
                }
                if (m_animationProperties.m_onLoopCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::RemoveCallback, m_animationProperties.m_onLoopCallbackId));
                }
                if (m_animationProperties.m_onUpdateCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::RemoveCallback, m_animationProperties.m_onUpdateCallbackId));
                }
            }
            else
            {
                m_timeSinceStart = .0f;
                if (m_animationProperties.m_onLoopCallbackId != AnimationProperties::InvalidCallbackId)
                {
                    callbacks.insert(CallbackData(CallbackTypes::OnLoop, m_animationProperties.m_onLoopCallbackId));
                }
            }
        }

        if (m_animationProperties.m_onUpdateCallbackId != AnimationProperties::InvalidCallbackId)
        {
            CallbackData updateCallback(CallbackTypes::OnUpdate, m_animationProperties.m_onUpdateCallbackId);
            GetValueAsAny(updateCallback.m_callbackData, currentValue);
            updateCallback.m_progressPercent = progressPercent;
            callbacks.insert(updateCallback);
        }

        if (m_animationProperties.m_isPlayingBackward)
        {
            deltaTime *= -1.0f;
        }
        m_timeSinceStart += (deltaTime * m_animationProperties.m_playbackSpeedMultiplier);
    }

    bool ScriptedEntityTweenerSubtask::CacheVirtualProperty(const AnimationParameterAddressData& animParamData)
    {
        /*
        Relies on some behavior context definitions for lookup

        behaviorContext->EBus<UiFaderBus>("UiFaderBus")
            ->Event("GetFadeValue", &UiFaderBus::Events::GetFadeValue)
            ->Event("SetFadeValue", &UiFaderBus::Events::SetFadeValue)
            ->VirtualProperty("Fade", "GetFadeValue", "SetFadeValue");

        behaviorContext->Class<UiFaderComponent>()->RequestBus("UiFaderBus");

        behaviorContext->EBus<UiFaderNotificationBus>("UiFaderNotificationBus")
            ->Handler<BehaviorUiFaderNotificationBusHandler>();
        */

        m_animParamData = animParamData;
        m_virtualProperty = nullptr;
        m_virtualPropertyTypeId = AZ::Uuid::CreateNull();

        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Error("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::CacheVirtualProperty - failed to get behavior context for caching [%s]", animParamData.m_virtualPropertyName.c_str());
            return false;
        }

        auto findClassIter = behaviorContext->m_classes.find(animParamData.m_componentName);
        if (findClassIter == behaviorContext->m_classes.end())
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::CacheVirtualProperty - failed to find behavior component class by component name [%s]", animParamData.m_componentName.c_str());
            return false;
        }

        AZ::BehaviorEBus::VirtualProperty* virtualProperty = nullptr;
        AZ::Uuid virtualPropertyTypeId = AZ::Uuid::CreateNull();

        // Get the virtual property
        AZ::BehaviorClass* behaviorClass = findClassIter->second;
        for (auto reqBusName = behaviorClass->m_requestBuses.begin(); reqBusName != behaviorClass->m_requestBuses.end(); reqBusName++)
        {
            auto findBusIter = behaviorContext->m_ebuses.find(*reqBusName);
            if (findBusIter != behaviorContext->m_ebuses.end())
            {
                AZ::BehaviorEBus* behaviorEbus = findBusIter->second;
                auto virtualPropertyIter = behaviorEbus->m_virtualProperties.find(animParamData.m_virtualPropertyName);
                if (virtualPropertyIter != behaviorEbus->m_virtualProperties.end())
                {
                    virtualProperty = &virtualPropertyIter->second;
                    break;
                }
            }
        }
        AZ_Warning("ScriptedEntityTweenerSubtask", virtualProperty, "ScriptedEntityTweenerSubtask::CacheVirtualProperty - failed to find virtual property by name [%s]", animParamData.m_virtualPropertyName.c_str());

        // Virtual properties with event setters/getters require a valid entityId
        if (virtualProperty)
        {
            if (((virtualProperty->m_setter && virtualProperty->m_setter->m_event)
                || (virtualProperty->m_getter && virtualProperty->m_getter->m_event))
                && !m_entityId.IsValid())
            {
                AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::CacheVirtualProperty - invalid entityId for virtual property's event setter/getter [%s, %s]", m_animParamData.m_componentName.c_str(), m_animParamData.m_virtualPropertyName.c_str());
                virtualProperty = nullptr;
            }
        }

        // Get the virtual property type
        if (virtualProperty)
        {
            if (virtualProperty->m_getter->m_event)
            {
                virtualPropertyTypeId = virtualProperty->m_getter->m_event->GetResult()->m_typeId;
            }
            else if (virtualProperty->m_getter->m_broadcast)
            {
                virtualPropertyTypeId = virtualProperty->m_getter->m_broadcast->GetResult()->m_typeId;
            }
            AZ_Warning("ScriptedEntityTweenerSubtask", !virtualPropertyTypeId.IsNull(), "ScriptedEntityTweenerSubtask::CacheVirtualProperty - failed to find virtual property type Id [%s]", animParamData.m_virtualPropertyName.c_str());
        }

        if (virtualProperty && !virtualPropertyTypeId.IsNull())
        {
            m_virtualProperty = virtualProperty;
            m_virtualPropertyTypeId = virtualPropertyTypeId;
            return true;
        }

        return false;
    }

    bool ScriptedEntityTweenerSubtask::IsVirtualPropertyCached()
    {
        return m_virtualProperty && !m_virtualPropertyTypeId.IsNull();
    }

    bool ScriptedEntityTweenerSubtask::GetValueFromAny(EntityAnimatedValue& value, const AZStd::any& anyValue)
    {
        if (!IsVirtualPropertyCached())
        {
            return false;
        }

        if (m_virtualPropertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float floatVal;
            if (any_numeric_cast(&anyValue, floatVal))
            {
                value.SetValue(floatVal);
            }
            else
            {
                AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetValueFromAny - numeric cast to float failed [%s]", m_animParamData.m_virtualPropertyName.c_str());
                return false;
            }
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid() && anyValue.is<AZ::Vector3>())
        {
            value.SetValue(AZStd::any_cast<AZ::Vector3>(anyValue));
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid() && anyValue.is<AZ::Color>())
        {
            AZ::Color color = AZStd::any_cast<AZ::Color>(anyValue);
            value.SetValue(color.GetAsVector3());
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid() && anyValue.is<AZ::Quaternion>())
        {
            value.SetValue(AZStd::any_cast<AZ::Quaternion>(anyValue));
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetValueFromAny - Virtual property type unsupported [%s]", m_animParamData.m_virtualPropertyName.c_str());
            return false;
        }
        return true;
    }

    bool ScriptedEntityTweenerSubtask::GetValueAsAny(AZStd::any& anyValue, const EntityAnimatedValue& value)
    {
        if (!IsVirtualPropertyCached())
        {
            return false;
        }

        if (m_virtualPropertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float floatVal;
            value.GetValue(floatVal);
            anyValue = floatVal;
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Vector3>::Uuid())
        {
            AZ::Vector3 vectorValue;
            value.GetValue(vectorValue);
            anyValue = vectorValue;
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Color>::Uuid())
        {
            AZ::Vector3 vectorValue;
            value.GetValue(vectorValue);
            anyValue = AZ::Color::CreateFromVector3(vectorValue);
        }
        else if (m_virtualPropertyTypeId == AZ::AzTypeInfo<AZ::Quaternion>::Uuid())
        {
            AZ::Quaternion quatValue;
            value.GetValue(quatValue);
            anyValue = quatValue;
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetValueAsAny - Virtual property type unsupported [%s]", m_animParamData.m_virtualPropertyName.c_str());
            return false;
        }
        return true;
    }

    bool ScriptedEntityTweenerSubtask::GetVirtualValue(EntityAnimatedValue& animatedValue)
    {
        if (!IsVirtualPropertyCached())
        {
            return false;
        }

        if (m_virtualPropertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float floatValue = 0.0f;
            SubtaskHelper::DoSafeGet(m_virtualProperty, m_entityId, floatValue);
            animatedValue.SetValue(floatValue);
        }
        else if (m_virtualPropertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
        {
            AZ::Vector3 vector3Value(AZ::Vector3::CreateZero());
            SubtaskHelper::DoSafeGet(m_virtualProperty, m_entityId, vector3Value);
            animatedValue.SetValue(vector3Value);
        }
        else if (m_virtualPropertyTypeId == AZ::Color::TYPEINFO_Uuid())
        {
            AZ::Color colorValue(AZ::Color::CreateZero());
            SubtaskHelper::DoSafeGet(m_virtualProperty, m_entityId, colorValue);
            AZ::Vector3 vector3Value = colorValue.GetAsVector3();
            animatedValue.SetValue(vector3Value);
        }
        else if (m_virtualPropertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
        {
            AZ::Quaternion quaternionValue(AZ::Quaternion::CreateIdentity());
            SubtaskHelper::DoSafeGet(m_virtualProperty, m_entityId, quaternionValue);
            animatedValue.SetValue(quaternionValue);
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetVirtualValue - Trying to get unsupported parameter type for [%s]", m_animParamData.m_virtualPropertyName.c_str());
            return false;
        }

        return true;
    }

    bool ScriptedEntityTweenerSubtask::SetVirtualValue(const EntityAnimatedValue& value)
    {
        if (!IsVirtualPropertyCached())
        {
            return false;
        }

        if (m_virtualPropertyTypeId == AZ::AzTypeInfo<float>::Uuid())
        {
            float floatValue;
            value.GetValue(floatValue);
            SubtaskHelper::DoSafeSet(m_virtualProperty, m_entityId, floatValue);
        }
        else if (m_virtualPropertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
        {
            AZ::Vector3 vector3Value;
            value.GetValue(vector3Value);
            SubtaskHelper::DoSafeSet(m_virtualProperty, m_entityId, vector3Value);
        }
        else if (m_virtualPropertyTypeId == AZ::Color::TYPEINFO_Uuid())
        {
            AZ::Vector3 vector3Value;
            value.GetValue(vector3Value);
            AZ::Color colorValue(AZ::Color::CreateFromVector3(vector3Value));
            SubtaskHelper::DoSafeSet(m_virtualProperty, m_entityId, colorValue);
        }
        else if (m_virtualPropertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
        {
            AZ::Quaternion quaternionValue;
            value.GetValue(quaternionValue);
            SubtaskHelper::DoSafeSet(m_virtualProperty, m_entityId, quaternionValue);
        }
        else
        {
            AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::SetVirtualValue - Trying to set unsupported parameter type for [%s]", m_animParamData.m_virtualPropertyName.c_str());
            return false;
        }

        return true;
    }

    bool ScriptedEntityTweenerSubtask::GetVirtualPropertyValue(AZStd::any& returnVal, const AnimationParameterAddressData& animParamData)
    {
        // If this is called before initialization, the virtual property needs to be cached.
        // This method is available on the ScriptedEntityTweenerBus to retrieve a virtual property value on an entity
        // regardless of whether a task/subtask for that entity/virtual property has been created (added to an animation).
        // In that circumstance, a temproary task/subtask is created, but not initialized
        if (!IsVirtualPropertyCached())
        {
            CacheVirtualProperty(animParamData);
        }

        if (IsVirtualPropertyCached())
        {
            EntityAnimatedValue tempVal;
            if (GetVirtualValue(tempVal))
            {
                return GetValueAsAny(returnVal, tempVal);
            }
        }

        AZ_Warning("ScriptedEntityTweenerSubtask", false, "ScriptedEntityTweenerSubtask::GetVirtualPropertyValue - failed for [%s, %s]", m_animParamData.m_componentName.c_str(), m_animParamData.m_virtualPropertyName.c_str());
        return false;
    }
}

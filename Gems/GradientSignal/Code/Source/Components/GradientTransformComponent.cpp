/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientTransformComponent.h"
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <AzCore/Component/TransformBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    void GradientTransformConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientTransformConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("TransformType", &GradientTransformConfig::m_transformType)
                ->Field("WrappingType", &GradientTransformConfig::m_wrappingType)
                ->Field("FrequencyZoom", &GradientTransformConfig::m_frequencyZoom)
                ->Field("AdvancedMode", &GradientTransformConfig::m_advancedMode)
                ->Field("Is3d", &GradientTransformConfig::m_is3d)
                ->Field("AllowReference", &GradientTransformConfig::m_allowReference)
                ->Field("ShapeReference", &GradientTransformConfig::m_shapeReference)
                ->Field("OverrideBounds", &GradientTransformConfig::m_overrideBounds)
                ->Field("Bounds", &GradientTransformConfig::m_bounds)
                ->Field("OverrideTranslate", &GradientTransformConfig::m_overrideTranslate)
                ->Field("Translate", &GradientTransformConfig::m_translate)
                ->Field("OverrideRotate", &GradientTransformConfig::m_overrideRotate)
                ->Field("Rotate", &GradientTransformConfig::m_rotate)
                ->Field("OverrideScale", &GradientTransformConfig::m_overrideScale)
                ->Field("Scale", &GradientTransformConfig::m_scale)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GradientTransformConfig>("Gradient Transform", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GradientTransformConfig::m_transformType, "Transform Type", "Determines how shape transforms are interpreted")
                    ->EnumAttribute(TransformType::World_Origin, "Origin")
                    ->EnumAttribute(TransformType::World_ThisEntity, "World Transform")
                    ->EnumAttribute(TransformType::Local_ThisEntity, "Relative to Parent")
                    ->EnumAttribute(TransformType::World_ReferenceEntity, "World Transform (of Reference)")
                    ->EnumAttribute(TransformType::Relative, "Relative to Reference")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GradientTransformConfig::m_wrappingType, "Wrapping Type", "")
                    ->EnumAttribute(WrappingType::None, "None (unbounded)")
                    ->EnumAttribute(WrappingType::ClampToEdge, "Clamp To Edge")
                    ->EnumAttribute(WrappingType::ClampToZero, "Clamp To Zero")
                    ->EnumAttribute(WrappingType::Mirror, "Mirror")
                    ->EnumAttribute(WrappingType::Repeat, "Repeat")

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientTransformConfig::m_frequencyZoom, "Frequency Zoom", "Rescales coordinates based on a multiplied factor")
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 8.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.25) // Give the frequency zoom a non-linear scale slider with higher precision at the low end

                    ->DataElement(0, &GradientTransformConfig::m_advancedMode, "Advanced Mode", "Enables advanced configuration options.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(0, &GradientTransformConfig::m_allowReference, "Allow Reference", "When enabled, the shape reference can be overridden. When disabled, all operations are relative to this entity.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsAdvancedModeReadOnly)
                    ->DataElement(0, &GradientTransformConfig::m_shapeReference, "Shape Reference", "An optional shape reference that can be used to drive bounds and transform")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsReferenceReadOnly)
                    
                    ->DataElement(0, &GradientTransformConfig::m_overrideBounds, "Override Bounds", "Allow manual override of the associated parameter")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsAdvancedModeReadOnly)
                    ->DataElement(0, &GradientTransformConfig::m_bounds, "Bounds", "Local (untransformed) bounds of a box used to remap, clamp, wrap, scale incoming coordinates")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsBoundsReadOnly)
                    
                    ->DataElement(0, &GradientTransformConfig::m_overrideTranslate, "Override Translate", "Allow manual override of the associated parameter")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsAdvancedModeReadOnly)
                    ->DataElement(0, &GradientTransformConfig::m_translate, "Translate", "")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsTranslateReadOnly)
                    ->DataElement(0, &GradientTransformConfig::m_overrideRotate, "Override Rotate", "Allow manual override of the associated parameter")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsAdvancedModeReadOnly)
                    ->DataElement(0, &GradientTransformConfig::m_rotate, "Rotate", "")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsRotateReadOnly)
                    ->DataElement(0, &GradientTransformConfig::m_overrideScale, "Override Scale", "Allow manual override of the associated parameter")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsAdvancedModeReadOnly)
                    ->DataElement(0, &GradientTransformConfig::m_scale, "Scale", "")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsScaleReadOnly)
                    
                    ->DataElement(0, &GradientTransformConfig::m_is3d, "Sample in 3D", "Check if the UVW should be mapped based on three dimensional world space")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientTransformConfig::IsAdvancedModeReadOnly)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<GradientTransformConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("advancedMode", BehaviorValueProperty(&GradientTransformConfig::m_advancedMode))
                ->Property("allowReference", BehaviorValueProperty(&GradientTransformConfig::m_allowReference))
                ->Property("shapeReference", BehaviorValueProperty(&GradientTransformConfig::m_shapeReference))
                ->Property("overrideBounds", BehaviorValueProperty(&GradientTransformConfig::m_overrideBounds))
                ->Property("bounds", BehaviorValueProperty(&GradientTransformConfig::m_bounds))
                ->Property("transformType",
                    [](GradientTransformConfig* config) { return (AZ::u8&)(config->m_transformType); },
                    [](GradientTransformConfig* config, const AZ::u8& i) { config->m_transformType = (TransformType)i; })
                ->Property("overrideTranslate", BehaviorValueProperty(&GradientTransformConfig::m_overrideTranslate))
                ->Property("translate", BehaviorValueProperty(&GradientTransformConfig::m_translate))
                ->Property("overrideRotate", BehaviorValueProperty(&GradientTransformConfig::m_overrideRotate))
                ->Property("rotate", BehaviorValueProperty(&GradientTransformConfig::m_rotate))
                ->Property("overrideScale", BehaviorValueProperty(&GradientTransformConfig::m_overrideScale))
                ->Property("scale", BehaviorValueProperty(&GradientTransformConfig::m_scale))
                ->Property("frequencyZoom", BehaviorValueProperty(&GradientTransformConfig::m_frequencyZoom))
                ->Property("wrappingType",
                    [](GradientTransformConfig* config) { return (AZ::u8&)(config->m_wrappingType); },
                    [](GradientTransformConfig* config, const AZ::u8& i) { config->m_wrappingType = (WrappingType)i; })
                ->Property("is3d", BehaviorValueProperty(&GradientTransformConfig::m_is3d))
                ;
        }
    }

    bool GradientTransformConfig::operator==(const GradientTransformConfig& other) const
    {
        return
            m_advancedMode == other.m_advancedMode &&
            m_allowReference == other.m_allowReference &&
            m_shapeReference == other.m_shapeReference &&
            m_overrideBounds == other.m_overrideBounds &&
            m_bounds.IsClose(other.m_bounds) &&
            m_transformType == other.m_transformType &&
            m_overrideTranslate == other.m_overrideTranslate &&
            m_translate.IsClose(other.m_translate) &&
            m_overrideRotate == other.m_overrideRotate &&
            m_rotate.IsClose(other.m_rotate) &&
            m_overrideScale == other.m_overrideScale &&
            m_scale.IsClose(other.m_scale) &&
            m_frequencyZoom == other.m_frequencyZoom &&
            m_wrappingType == other.m_wrappingType &&
            m_is3d == other.m_is3d;
    }

    bool GradientTransformConfig::operator!=(const GradientTransformConfig& other) const
    {
        return !(*this == other);
    }

    bool GradientTransformConfig::IsAdvancedModeReadOnly() const
    {
        return !m_advancedMode;
    }

    bool GradientTransformConfig::IsReferenceReadOnly() const
    {
        return !m_allowReference || !m_advancedMode;
    }

    bool GradientTransformConfig::IsBoundsReadOnly() const
    {
        return !m_overrideBounds || !m_advancedMode;
    }

    bool GradientTransformConfig::IsTranslateReadOnly() const
    {
        return !m_overrideTranslate || !m_advancedMode;
    }

    bool GradientTransformConfig::IsRotateReadOnly() const
    {
        return !m_overrideRotate || !m_advancedMode;
    }

    bool GradientTransformConfig::IsScaleReadOnly() const
    {
        return !m_overrideScale || !m_advancedMode;
    }

    void GradientTransformComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void GradientTransformComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
        services.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void GradientTransformComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void GradientTransformComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientTransformConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientTransformComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &GradientTransformComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("GradientTransformComponentTypeId", BehaviorConstant(GradientTransformComponentTypeId));

            behaviorContext->Class<GradientTransformComponent>()->RequestBus("GradientTransformModifierRequestBus");

            behaviorContext->EBus<GradientTransformModifierRequestBus>("GradientTransformModifierRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowReference", &GradientTransformModifierRequestBus::Events::GetAllowReference)
                ->Event("SetAllowReference", &GradientTransformModifierRequestBus::Events::SetAllowReference)
                ->VirtualProperty("AllowReference", "GetAllowReference", "SetAllowReference")
                ->Event("GetShapeReference", &GradientTransformModifierRequestBus::Events::GetShapeReference)
                ->Event("SetShapeReference", &GradientTransformModifierRequestBus::Events::SetShapeReference)
                ->VirtualProperty("ShapeReference", "GetShapeReference", "SetShapeReference")
                ->Event("GetOverrideBounds", &GradientTransformModifierRequestBus::Events::GetOverrideBounds)
                ->Event("SetOverrideBounds", &GradientTransformModifierRequestBus::Events::SetOverrideBounds)
                ->VirtualProperty("OverrideBounds", "GetOverrideBounds", "SetOverrideBounds")
                ->Event("GetBounds", &GradientTransformModifierRequestBus::Events::GetBounds)
                ->Event("SetBounds", &GradientTransformModifierRequestBus::Events::SetBounds)
                ->VirtualProperty("Bounds", "GetBounds", "SetBounds")
                ->Event("GetTransformType", &GradientTransformModifierRequestBus::Events::GetTransformType)
                ->Event("SetTransformType", &GradientTransformModifierRequestBus::Events::SetTransformType)
                ->VirtualProperty("TransformType", "GetTransformType", "SetTransformType")
                ->Event("GetOverrideTranslate", &GradientTransformModifierRequestBus::Events::GetOverrideTranslate)
                ->Event("SetOverrideTranslate", &GradientTransformModifierRequestBus::Events::SetOverrideTranslate)
                ->VirtualProperty("OverrideTranslate", "GetOverrideTranslate", "SetOverrideTranslate")
                ->Event("GetTranslate", &GradientTransformModifierRequestBus::Events::GetTranslate)
                ->Event("SetTranslate", &GradientTransformModifierRequestBus::Events::SetTranslate)
                ->VirtualProperty("Translate", "GetTranslate", "SetTranslate")
                ->Event("GetOverrideRotate", &GradientTransformModifierRequestBus::Events::GetOverrideRotate)
                ->Event("SetOverrideRotate", &GradientTransformModifierRequestBus::Events::SetOverrideRotate)
                ->VirtualProperty("OverrideRotate", "GetOverrideRotate", "SetOverrideRotate")
                ->Event("GetRotate", &GradientTransformModifierRequestBus::Events::GetRotate)
                ->Event("SetRotate", &GradientTransformModifierRequestBus::Events::SetRotate)
                ->VirtualProperty("Rotate", "GetRotate", "SetRotate")
                ->Event("GetOverrideScale", &GradientTransformModifierRequestBus::Events::GetOverrideScale)
                ->Event("SetOverrideScale", &GradientTransformModifierRequestBus::Events::SetOverrideScale)
                ->VirtualProperty("OverrideScale", "GetOverrideScale", "SetOverrideScale")
                ->Event("GetScale", &GradientTransformModifierRequestBus::Events::GetScale)
                ->Event("SetScale", &GradientTransformModifierRequestBus::Events::SetScale)
                ->VirtualProperty("Scale", "GetScale", "SetScale")
                ->Event("GetFrequencyZoom", &GradientTransformModifierRequestBus::Events::GetFrequencyZoom)
                ->Event("SetFrequencyZoom", &GradientTransformModifierRequestBus::Events::SetFrequencyZoom)
                ->VirtualProperty("FrequencyZoom", "GetFrequencyZoom", "SetFrequencyZoom")
                ->Event("GetWrappingType", &GradientTransformModifierRequestBus::Events::GetWrappingType)
                ->Event("SetWrappingType", &GradientTransformModifierRequestBus::Events::SetWrappingType)
                ->VirtualProperty("WrappingType", "GetWrappingType", "SetWrappingType")
                ->Event("GetIs3D", &GradientTransformModifierRequestBus::Events::GetIs3D)
                ->Event("SetIs3D", &GradientTransformModifierRequestBus::Events::SetIs3D)
                ->VirtualProperty("Is3D", "GetIs3D", "SetIs3D")
                ;
        }
    }

    GradientTransformComponent::GradientTransformComponent(const GradientTransformConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void GradientTransformComponent::Activate()
    {
        GradientTransformRequestBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();
        GradientTransformModifierRequestBus::Handler::BusConnect(GetEntityId());

        m_dirty = false;

        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetShapeEntityId());

        UpdateFromShape();
    }

    void GradientTransformComponent::Deactivate()
    {
        m_dirty = false;

        m_dependencyMonitor.Reset();
        GradientTransformRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        GradientTransformModifierRequestBus::Handler::BusDisconnect();
    }

    bool GradientTransformComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const GradientTransformConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool GradientTransformComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<GradientTransformConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void GradientTransformComponent::TransformPositionToUVW(const AZ::Vector3& inPosition, AZ::Vector3& outUVW, const bool shouldNormalizeOutput, bool& wasPointRejected) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        //transforming coordinate into "local" relative space of shape bounds
        outUVW = m_shapeTransformInverse * inPosition;

        if (!m_configuration.m_advancedMode || !m_configuration.m_is3d)
        {
            outUVW.SetZ(0.0f);
        }

        wasPointRejected = false;
        if (m_shapeBounds.IsValid())
        {
            //all wrap types and transformations are applied after the coordinate is transformed into shape relative space
            //this allows all calculations to be simplified and done using the shapes untransformed aabb
            //outputting a value that can be used to sample a gradient in its local space
            switch (m_configuration.m_wrappingType)
            {
            default:
            case WrappingType::None:
                outUVW = GetUnboundedPointInAabb(outUVW, m_shapeBounds);
                break;
            case WrappingType::ClampToEdge:
                outUVW = GetClampedPointInAabb(outUVW, m_shapeBounds);
                break;
            case WrappingType::ClampToZero:
                // We don't want to use m_shapeBounds.Contains() here because Contains() is inclusive on all edges.
                // For uv consistency between clamped and unclamped states, we only want to accept uv ranges of [min, max), 
                // so we specifically need to exclude the max edges here.
                wasPointRejected = !(outUVW.IsGreaterEqualThan(m_shapeBounds.GetMin()) && outUVW.IsLessThan(m_shapeBounds.GetMax()));
                outUVW = GetClampedPointInAabb(outUVW, m_shapeBounds);
                break;
            case WrappingType::Mirror:
                outUVW = GetMirroredPointInAabb(outUVW, m_shapeBounds);
                break;
            case WrappingType::Repeat:
                outUVW = GetWrappedPointInAabb(outUVW, m_shapeBounds);
                break;
            }
        }

        outUVW *= m_configuration.m_frequencyZoom;

        if (shouldNormalizeOutput)
        {
            outUVW = GetNormalizedPointInAabb(outUVW, m_shapeBounds);
        }
    }

    void GradientTransformComponent::GetGradientLocalBounds(AZ::Aabb& bounds) const
    {
        bounds = m_shapeBounds;
    }

    void GradientTransformComponent::GetGradientEncompassingBounds(AZ::Aabb& bounds) const
    {
        bounds = m_shapeBounds;
        bounds.ApplyMatrix3x4(m_shapeTransformInverse.GetInverseFull());
    }

    void GradientTransformComponent::OnCompositionChanged()
    {
        m_dirty = true;
    }

    void GradientTransformComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_dirty)
        {
            const auto configurationOld = m_configuration;
            const auto shapeBoundsOld = m_shapeBounds;
            const auto shapeTransformInverseOld = m_shapeTransformInverse;

            //updating on tick to query transform bus on main thread
            UpdateFromShape();

            //notify observers if content has changed
            if (configurationOld != m_configuration ||
                shapeBoundsOld != m_shapeBounds ||
                shapeTransformInverseOld != m_shapeTransformInverse)
            {
                LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
            }
            m_dirty = false;
        }
    }

    void GradientTransformComponent::UpdateFromShape()
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

        AZ::EntityId shapeReference = GetShapeEntityId();
        if (!shapeReference.IsValid())
        {
            return;
        }

        AZ::Transform shapeTransform = AZ::Transform::CreateIdentity();
        switch (m_configuration.m_transformType)
        {
            default:
            case TransformType::World_Origin:
            {
                break;
            }
            case TransformType::Local_ThisEntity:
            {
                AZ::TransformBus::EventResult(shapeTransform, GetEntityId(), &AZ::TransformBus::Events::GetLocalTM);
                break;
            }
            case TransformType::World_ThisEntity:
            {
                AZ::TransformBus::EventResult(shapeTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                break;
            }
            case TransformType::Local_ReferenceEntity:
            {
                AZ::TransformBus::EventResult(shapeTransform, shapeReference, &AZ::TransformBus::Events::GetLocalTM);
                break;
            }
            case TransformType::World_ReferenceEntity:
            {
                AZ::TransformBus::EventResult(shapeTransform, shapeReference, &AZ::TransformBus::Events::GetWorldTM);
                break;
            }
            case TransformType::Relative:
            {
                AZ::Transform entityWorldTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(entityWorldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                AZ::Transform referenceWorldTransform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(referenceWorldTransform, shapeReference, &AZ::TransformBus::Events::GetWorldTM);
                shapeTransform = referenceWorldTransform.GetInverse() * entityWorldTransform;
                break;
            }
        }

        // only update shape bounds with a responsive Entity reference
        if (!m_configuration.m_advancedMode || !m_configuration.m_overrideBounds)
        {
            // If we have a shape reference, grab its local space bounds and (inverse) transform into that local space
            GetObbParamsFromShape(shapeReference, m_shapeBounds, m_shapeTransformInverse);
            if (m_shapeBounds.IsValid())
            {
                m_configuration.m_bounds = m_shapeBounds.GetExtents();
            }
        }

        //support overriding/ignoring elements of transform
        if (!m_configuration.m_advancedMode || !m_configuration.m_overrideTranslate)
        {
            m_configuration.m_translate = shapeTransform.GetTranslation();
        }

        if (!m_configuration.m_advancedMode || !m_configuration.m_overrideRotate)
        {
            m_configuration.m_rotate = shapeTransform.GetRotation().GetEulerDegrees();
        }

        if (!m_configuration.m_advancedMode || !m_configuration.m_overrideScale)
        {
            m_configuration.m_scale = AZ::Vector3(shapeTransform.GetUniformScale());
        }

        //rebuild bounds from parameters
        m_configuration.m_bounds = m_configuration.m_bounds.GetAbs();
        m_shapeBounds = AZ::Aabb::CreateFromMinMax(-m_configuration.m_bounds * 0.5f, m_configuration.m_bounds * 0.5f);

        //rebuild transform from parameters
        AZ::Matrix3x4 shapeTransformFinal;
        shapeTransformFinal.SetFromEulerDegrees(m_configuration.m_rotate);
        shapeTransformFinal.SetTranslation(m_configuration.m_translate);
        shapeTransformFinal.MultiplyByScale(m_configuration.m_scale);
        m_shapeTransformInverse = shapeTransformFinal.GetInverseFull();
    }

    AZ::EntityId GradientTransformComponent::GetShapeEntityId() const
    {
        return m_configuration.m_advancedMode && m_configuration.m_allowReference && m_configuration.m_shapeReference.IsValid() ? m_configuration.m_shapeReference : GetEntityId();
    }

    bool GradientTransformComponent::GetAllowReference() const
    {
        return m_configuration.m_allowReference;
    }
    
    void GradientTransformComponent::SetAllowReference(bool value)
    {
        m_configuration.m_allowReference = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::EntityId GradientTransformComponent::GetShapeReference() const
    {
        return m_configuration.m_shapeReference;
    }
    
    void GradientTransformComponent::SetShapeReference(AZ::EntityId shapeReference)
    {
        m_configuration.m_shapeReference = shapeReference;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool GradientTransformComponent::GetOverrideBounds() const
    {
        return m_configuration.m_overrideBounds;
    }
    
    void GradientTransformComponent::SetOverrideBounds(bool value)
    {
        m_configuration.m_overrideBounds = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 GradientTransformComponent::GetBounds() const
    {
        return m_configuration.m_bounds;
    }
    
    void GradientTransformComponent::SetBounds(AZ::Vector3 bounds)
    {
        m_configuration.m_bounds = bounds;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    TransformType GradientTransformComponent::GetTransformType() const
    {
        return m_configuration.m_transformType;
    }
    
    void GradientTransformComponent::SetTransformType(TransformType type)
    {
        m_configuration.m_transformType = type;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool GradientTransformComponent::GetOverrideTranslate() const
    {
        return m_configuration.m_overrideTranslate;
    }
    
    void GradientTransformComponent::SetOverrideTranslate(bool value)
    {
        m_configuration.m_overrideTranslate = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 GradientTransformComponent::GetTranslate() const
    {
        return m_configuration.m_translate;
    }
    
    void GradientTransformComponent::SetTranslate(AZ::Vector3 translate)
    {
        m_configuration.m_translate = translate;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool GradientTransformComponent::GetOverrideRotate() const
    {
        return m_configuration.m_overrideRotate;
    }
    
    void GradientTransformComponent::SetOverrideRotate(bool value)
    {
        m_configuration.m_overrideRotate = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 GradientTransformComponent::GetRotate() const
    {
        return m_configuration.m_rotate;
    }
    
    void GradientTransformComponent::SetRotate(AZ::Vector3 rotate)
    {
        m_configuration.m_rotate = rotate;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool GradientTransformComponent::GetOverrideScale() const
    {
        return m_configuration.m_overrideScale;
    }
    
    void GradientTransformComponent::SetOverrideScale(bool value)
    {
        m_configuration.m_overrideScale = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 GradientTransformComponent::GetScale() const
    {
        return m_configuration.m_scale;
    }
    
    void GradientTransformComponent::SetScale(AZ::Vector3 scale)
    {
        m_configuration.m_scale = scale;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float GradientTransformComponent::GetFrequencyZoom() const
    {
        return m_configuration.m_frequencyZoom;
    }
    
    void GradientTransformComponent::SetFrequencyZoom(float frequencyZoom)
    {
        m_configuration.m_frequencyZoom = frequencyZoom;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    WrappingType GradientTransformComponent::GetWrappingType() const
    {
        return m_configuration.m_wrappingType;
    }
    
    void GradientTransformComponent::SetWrappingType(WrappingType type)
    {
        m_configuration.m_wrappingType = type;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool GradientTransformComponent::GetIs3D() const
    {
        return m_configuration.m_is3d;
    }

    void GradientTransformComponent::SetIs3D(bool value)
    {
        m_configuration.m_is3d = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    bool GradientTransformComponent::GetAdvancedMode() const
    {
        return m_configuration.m_advancedMode;
    }

    void GradientTransformComponent::SetAdvancedMode(bool value)
    {
        m_configuration.m_advancedMode = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

} //namespace GradientSignal

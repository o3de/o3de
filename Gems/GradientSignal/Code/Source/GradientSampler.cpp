/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/GradientSampler.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    void GradientSampler::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientSampler>()
                ->Version(1)
                ->Field("GradientId", &GradientSampler::m_gradientId)
                ->Field("Opacity", &GradientSampler::m_opacity)
                ->Field("InvertInput", &GradientSampler::m_invertInput)
                ->Field("EnableTransform", &GradientSampler::m_enableTransform)
                ->Field("Translate", &GradientSampler::m_translate)
                ->Field("Scale", &GradientSampler::m_scale)
                ->Field("Rotate", &GradientSampler::m_rotate)
                ->Field("EnableLevels", &GradientSampler::m_enableLevels)
                ->Field("InputMid", &GradientSampler::m_inputMid)
                ->Field("InputMin", &GradientSampler::m_inputMin)
                ->Field("InputMax", &GradientSampler::m_inputMax)
                ->Field("OutputMin", &GradientSampler::m_outputMin)
                ->Field("OutputMax", &GradientSampler::m_outputMax)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<GradientSampler>("Gradient Sampler", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &GradientSampler::m_gradientId, "Gradient Entity Id", "Entity with attached gradient component")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("GradientService"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GradientSampler::ChangeNotify)
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &GradientSampler::ValidatePotentialEntityId)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSampler::m_opacity, "Opacity", "Factor multiplied by the current gradient before mixing.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GradientSampler::ChangeNotify)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(0, &GradientSampler::m_invertInput, "Invert Input", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GradientSampler::ChangeNotify)

                    ->GroupElementToggle("Enable Transform", &GradientSampler::m_enableTransform)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(0, &GradientSampler::m_translate, "Translate", "")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreTransformSettingsDisabled)
                    ->DataElement(0, &GradientSampler::m_scale, "Scale", "")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreTransformSettingsDisabled)
                    ->DataElement(0, &GradientSampler::m_rotate, "Rotate", "Rotation in degrees.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreTransformSettingsDisabled)

                    ->GroupElementToggle("Enable Levels", &GradientSampler::m_enableLevels)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSampler::m_inputMid, "Input Mid", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreLevelSettingsDisabled)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSampler::m_inputMin, "Input Min", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreLevelSettingsDisabled)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSampler::m_inputMax, "Input Max", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreLevelSettingsDisabled)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSampler::m_outputMin, "Output Min", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreLevelSettingsDisabled)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSampler::m_outputMax, "Output Max", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientSampler::AreLevelSettingsDisabled)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview (Inbound)")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->UIElement("GradientPreviewer", "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientSampler"), &GradientSampler::GetSampler)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<GradientSampler>()
                ->Constructor()
                ->Property("gradientId", BehaviorValueProperty(&GradientSampler::m_gradientId))
                ->Property("opacity", BehaviorValueProperty(&GradientSampler::m_opacity))
                ->Property("invertInput", BehaviorValueProperty(&GradientSampler::m_invertInput))
                ->Property("enableLevels", BehaviorValueProperty(&GradientSampler::m_enableLevels))
                ->Property("inputMid", BehaviorValueProperty(&GradientSampler::m_inputMid))
                ->Property("inputMin", BehaviorValueProperty(&GradientSampler::m_inputMin))
                ->Property("inputMax", BehaviorValueProperty(&GradientSampler::m_inputMax))
                ->Property("outputMin", BehaviorValueProperty(&GradientSampler::m_outputMin))
                ->Property("outputMax", BehaviorValueProperty(&GradientSampler::m_outputMax))
                ->Property("enableTransforms", BehaviorValueProperty(&GradientSampler::m_enableTransform))
                ->Property("translation", BehaviorValueProperty(&GradientSampler::m_translate))
                ->Property("scale", BehaviorValueProperty(&GradientSampler::m_scale))
                ->Property("rotation", BehaviorValueProperty(&GradientSampler::m_rotate))
                ;
        }
    }

    GradientSampler* GradientSampler::GetSampler()
    {
        return this;
    }

    AZ::u32 GradientSampler::ChangeNotify() const
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::Outcome<void, AZStd::string> GradientSampler::ValidatePotentialEntityId(void* newValue, const AZ::Uuid& valueType) const
    {
        if (azrtti_typeid<AZ::EntityId>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Trying to set an entity ID to something that isn't an entity ID!"));
        }

        AZ::EntityId potentialEntityId = static_cast<AZ::EntityId>(*((AZ::EntityId*)newValue));

        // Always allow a clear, no need to check
        if (!potentialEntityId.IsValid())
        {
            return AZ::Success();
        }

        // Prevent setting the parent to the entity itself.
        if (potentialEntityId == m_ownerEntityId)
        {
            return AZ::Failure(AZStd::string("You cannot set a gradient or modifier to point to itself!"));
        }
        else
        {
            bool loopCreated = false;

            // See if we are in the new connections network already
            GradientRequestBus::EventResult(loopCreated, potentialEntityId, &GradientRequestBus::Events::IsEntityInHierarchy, m_ownerEntityId);

            if (loopCreated)
            {
                return AZ::Failure(AZStd::string("Setting this entity reference will cause a cyclical loop, which is not allowed!"));
            }
        }

        return AZ::Success();
    }

    bool GradientSampler::ValidateGradientEntityId()
    {
        AZ::ComponentValidationResult result = ValidatePotentialEntityId(&m_gradientId, azrtti_typeid<AZ::EntityId>());
        if (!result.IsSuccess())
        {
            AZ_Warning("GradientSignal", false, "Gradient Sampler refers to an entity that will cause a cyclical loop, which is not allowed!  Clearing gradient entity id!");
            m_gradientId = AZ::EntityId();
            return false;
        }

        return true;
    }


    bool GradientSampler::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        if (entityId == m_gradientId)
        {
            return true;
        }

        bool inHierarchy = false;

        GradientRequestBus::EventResult(inHierarchy, m_gradientId, &GradientRequestBus::Events::IsEntityInHierarchy, entityId);

        return inHierarchy;
    }

    AZ::Aabb GradientSampler::TransformDirtyRegion(const AZ::Aabb& dirtyRegion) const
    {
        if ((!m_enableTransform) || (!dirtyRegion.IsValid()))
        {
            return dirtyRegion;
        }

        // We do *not* use the inverse transform here because we're transforming from world space to world space.
        AZ::Matrix3x4 transformMatrix = GetTransformMatrix();

        return dirtyRegion.GetTransformedAabb(transformMatrix);
    }

    bool GradientSampler::AreLevelSettingsDisabled() const
    {
        return !m_enableLevels;
    }

    bool GradientSampler::AreTransformSettingsDisabled() const
    {
        return !m_enableTransform;
    }
}

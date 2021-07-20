/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/Util.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>

namespace GradientSignal
{
    struct GradientSampleParams;

    class GradientSampler final
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientSampler, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientSampler, "{3768D3A6-BF70-4ABC-B4EC-73C75A886916}");
        static void Reflect(AZ::ReflectContext* context);

        inline float GetValue(const GradientSampleParams& sampleParams) const;

        bool IsEntityInHierarchy(const AZ::EntityId& entityId) const;

        AZ::EntityId m_gradientId;
        //! Entity that owns the gradientSampler itself, used by the gradient previewer
        AZ::EntityId m_ownerEntityId;
        float m_opacity = 1.0f;

        bool m_invertInput = false;

        bool m_enableTransform = false;
        AZ::Vector3 m_translate = AZ::Vector3::CreateZero();
        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
        AZ::Vector3 m_rotate = AZ::Vector3::CreateZero();

        //embedded levels controls
        bool m_enableLevels = false;
        float m_inputMid = 1.0f;
        float m_inputMin = 0.0f;
        float m_inputMax = 1.0f;
        float m_outputMin = 0.0f;
        float m_outputMax = 1.0f;

        bool ValidateGradientEntityId();

    private:
        // Pass-through for UIElement attribute
        GradientSampler* GetSampler();
        AZ::u32 ChangeNotify() const;
        bool AreLevelSettingsDisabled() const;
        bool AreTransformSettingsDisabled() const;

        AZ::Outcome<void, AZStd::string>  ValidatePotentialEntityId(void* newValue, const AZ::Uuid& valueType) const;

        //prevent recursion in case user attaches cyclic dependences
        mutable bool m_isRequestInProgress = false; 
    };

    namespace GradientSamplerUtil
    {
        AZ_INLINE bool AreLevelParamsSet(const GradientSampler& sampler)
        {
            return sampler.m_inputMid != 1.0f || sampler.m_inputMin != 0.0f || sampler.m_inputMax != 1.0f || sampler.m_outputMin != 0.0f || sampler.m_outputMax != 1.0f;
        }

        AZ_INLINE bool AreTransformParamsSet(const GradientSampler& sampler)
        {
            static const AZ::Vector3 zero3 = AZ::Vector3::CreateZero();
            static const AZ::Vector3 one3 = AZ::Vector3::CreateOne();
            return sampler.m_translate != zero3 || sampler.m_rotate != zero3 || sampler.m_scale != one3;
        }
    }

    inline float GradientSampler::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        if (m_opacity <= 0.0f || !m_gradientId.IsValid())
        {
            return 0.0f;
        }

        GradientSampleParams sampleParamsTransformed(sampleParams);

        //apply transform if set
        if (m_enableTransform && GradientSamplerUtil::AreTransformParamsSet(*this))
        {
            AZ::Matrix3x4 matrix3x4;
            matrix3x4.SetFromEulerDegrees(m_rotate);
            matrix3x4.MultiplyByScale(m_scale);
            matrix3x4.SetTranslation(m_translate);

            sampleParamsTransformed.m_position = matrix3x4 * sampleParamsTransformed.m_position;
        }

        float output = 0.0f;

        {
            // Block other threads from accessing the surface data bus while we are in GetValue (which may call into the SurfaceData bus).
            // We lock our surface data mutex *before* checking / setting "isRequestInProgress" so that we prevent race conditions
            // that create false detection of cyclic dependencies when multiple requests occur on different threads simultaneously.
            // (One case where this was previously able to occur was in rapid updating of the Preview widget on the GradientSurfaceDataComponent
            // in the Editor when moving the threshold sliders back and forth rapidly)
            auto& surfaceDataContext = SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext(false);
            typename SurfaceData::SurfaceDataSystemRequestBus::Context::DispatchLockGuard scopeLock(surfaceDataContext.m_contextMutex);

            if (m_isRequestInProgress)
            {
                AZ_ErrorOnce("GradientSignal", !m_isRequestInProgress, "Detected cyclic dependences with gradient entity references");
            }
            else
            {
                m_isRequestInProgress = true;

                GradientRequestBus::EventResult(output, m_gradientId, &GradientRequestBus::Events::GetValue, sampleParamsTransformed);

                if (m_invertInput)
                {
                    output = 1.0f - output;
                }

                //apply levels if set
                if (m_enableLevels && GradientSamplerUtil::AreLevelParamsSet(*this))
                {
                    output = GetLevels(output, m_inputMid, m_inputMin, m_inputMax, m_outputMin, m_outputMax);
                }

                m_isRequestInProgress = false;
            }

        }

        return output * m_opacity;
    }
}

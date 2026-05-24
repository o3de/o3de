/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Window/Controls/DistIndexUtil.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>

namespace OpenParticleSystemEditor
{
    template<typename ValueType>
    size_t GetIndex(void* address, int index, const OpenParticle::DistributionType& distType)
    {
        ValueType* valueObj = static_cast<ValueType*>(address);
        if (index < static_cast<int>(valueObj->Size()) && valueObj->distType == distType)
        {
            return valueObj->distIndex[index];
        }
        return 0;
    }

    template<>
    size_t GetIndex<OpenParticle::ValueObjLinear>(void* address, int index, const OpenParticle::DistributionType& distType)
    {
        OpenParticle::ValueObjLinear* valueObj = static_cast<OpenParticle::ValueObjLinear*>(address);
        if (distType == OpenParticle::DistributionType::RANDOM)
        {
            return 0;
        }
        if (index < static_cast<int>(valueObj->Size()) && valueObj->distType == distType)
        {
            return valueObj->distIndex[index];
        }
        return 0;
    }

    size_t DistIndexUtil::GetDistIndex(
        void* address, const AZ::TypeId& id, const OpenParticle::DistributionType& distType, int index)
    {
        if (address == nullptr)
        {
            return 0;
        }

        if (id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            return GetIndex<OpenParticle::ValueObjFloat>(address, index, distType);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            return GetIndex<OpenParticle::ValueObjVec2>(address, index, distType);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            return GetIndex<OpenParticle::ValueObjVec3>(address, index, distType);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            return GetIndex<OpenParticle::ValueObjVec4>(address, index, distType);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            return GetIndex<OpenParticle::ValueObjColor>(address, index, distType);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            return GetIndex<OpenParticle::ValueObjLinear>(address, index, distType);
        }
        return 0;
    }

    template<typename ValueType>
    void SetIndex(void* address, int index, const OpenParticle::DistributionType& distType, size_t value)
    {
        ValueType* valueObj = static_cast<ValueType*>(address);
        if (index < static_cast<int>(valueObj->Size()))
        {
            valueObj->distType = distType;
            valueObj->distIndex[index] = static_cast<AZ::u32>(value);
        }
    }

    template<>
    void SetIndex<OpenParticle::ValueObjLinear>(void* address, int index, const OpenParticle::DistributionType& distType, size_t value)
    {
        OpenParticle::ValueObjLinear* valueObj = static_cast<OpenParticle::ValueObjLinear*>(address);
        if (index >= static_cast<int>(valueObj->Size()) || distType == OpenParticle::DistributionType::RANDOM)
        {
            return;
        }
        valueObj->distType = distType;
        valueObj->distIndex[index] = static_cast<AZ::u32>(value);

    }

    void DistIndexUtil::SetDistIndex(
        void* address, const AZ::TypeId& id, const OpenParticle::DistributionType& distType, size_t value, int index)
    {
        if (address == nullptr)
        {
            return;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            SetIndex<OpenParticle::ValueObjFloat>(address, index, distType, value);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            SetIndex<OpenParticle::ValueObjVec2>(address, index, distType, value);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            SetIndex<OpenParticle::ValueObjVec3>(address, index, distType, value);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            SetIndex<OpenParticle::ValueObjVec4>(address, index, distType, value);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            SetIndex<OpenParticle::ValueObjColor>(address, index, distType, value);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            SetIndex<OpenParticle::ValueObjLinear>(address, index, distType, value);
        }
    }

    template<typename ValueType>
    void ClearIndex(void* address)
    {
        ValueType* valueObj = static_cast<ValueType*>(address);
        valueObj->distType = OpenParticle::DistributionType::CONSTANT;
        for (AZ::u32 index = 0; index < valueObj->Size(); ++index)
        {
            valueObj->distIndex[index] = 0;
        }
    }

    void DistIndexUtil::ClearDistIndex(void* address, const AZ::TypeId& id)
    {
        if (address == nullptr)
        {
            return;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            ClearIndex<OpenParticle::ValueObjFloat>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            ClearIndex<OpenParticle::ValueObjVec2>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            ClearIndex<OpenParticle::ValueObjVec3>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            ClearIndex<OpenParticle::ValueObjVec4>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            ClearIndex<OpenParticle::ValueObjColor>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            ClearIndex<OpenParticle::ValueObjLinear>(address);
        }
    }

    template<typename ValueType>
    OpenParticle::DistributionType GetDistType(void* address)
    {
        ValueType* valueObj = static_cast<ValueType*>(address);
        return valueObj->distType;
    }

    OpenParticle::DistributionType DistIndexUtil::GetDistributionType(void* address, const AZ::TypeId& id)
    {
        if (address == nullptr)
        {
            return OpenParticle::DistributionType::CONSTANT;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            return GetDistType<OpenParticle::ValueObjFloat>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            return GetDistType<OpenParticle::ValueObjVec2>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            return GetDistType<OpenParticle::ValueObjVec3>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            return GetDistType<OpenParticle::ValueObjVec4>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            return GetDistType<OpenParticle::ValueObjColor>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            return GetDistType<OpenParticle::ValueObjLinear>(address);
        }
        return OpenParticle::DistributionType::CONSTANT;
    }

    template<typename ValueType>
    void SetDistType(void* address, const OpenParticle::DistributionType& type)
    {
        ValueType* valueObj = static_cast<ValueType*>(address);
        valueObj->distType = type;
        if (type == OpenParticle::DistributionType::CONSTANT)
        {
            for (auto& it : valueObj->distIndex)
            {
                it = 0;
            }
        }
    }

    void DistIndexUtil::SetDistributionType(void* address, const AZ::TypeId& id, const OpenParticle::DistributionType& type)
    {
        if (address == nullptr)
        {
            return;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            SetDistType<OpenParticle::ValueObjFloat>(address, type);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            SetDistType<OpenParticle::ValueObjVec2>(address, type);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            SetDistType<OpenParticle::ValueObjVec3>(address, type);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            SetDistType<OpenParticle::ValueObjVec4>(address, type);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            SetDistType<OpenParticle::ValueObjColor>(address, type);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            SetDistType<OpenParticle::ValueObjLinear>(address, type);
        }
    }

    template<typename ValueType>
    bool IsUniform(void* address)
    {
        ValueType* valueObj = static_cast<ValueType*>(address);
        return valueObj->isUniform;
    }

    bool DistIndexUtil::IsUinform(void* address, const AZ::TypeId& id)
    {
        if (address == nullptr || id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            return false;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            return IsUniform<OpenParticle::ValueObjVec2>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            return IsUniform<OpenParticle::ValueObjVec3>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            return IsUniform<OpenParticle::ValueObjVec4>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            return IsUniform<OpenParticle::ValueObjColor>(address);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            return IsUniform<OpenParticle::ValueObjLinear>(address);
        }
        return false;
    }

    template<typename ValueType>
    void SetUniform(void* address, bool uniform)
    {
        ValueType* valueObj = static_cast<ValueType*>(address);
        valueObj->isUniform = uniform;
    }

    void DistIndexUtil::SetUinform(void* address, const AZ::TypeId& id, bool isUniform)
    {
        if (address == nullptr || id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            return;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            SetUniform<OpenParticle::ValueObjVec2>(address, isUniform);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            SetUniform<OpenParticle::ValueObjVec3>(address, isUniform);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>())
        {
            SetUniform<OpenParticle::ValueObjVec4>(address, isUniform);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            SetUniform<OpenParticle::ValueObjColor>(address, isUniform);
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            SetUniform<OpenParticle::ValueObjLinear>(address, isUniform);
        }
    }
} // namespace OpenParticleSystemEditor

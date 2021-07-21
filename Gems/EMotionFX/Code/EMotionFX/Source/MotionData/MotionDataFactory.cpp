/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/MotionData/MotionDataFactory.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>

namespace EMotionFX
{
    MotionDataFactory::~MotionDataFactory()
    {
        Clear();
    }

    void MotionDataFactory::Init()
    {
        Register(aznew UniformMotionData());
        Register(aznew NonUniformMotionData());
    }

    void MotionDataFactory::Clear()
    {
        const size_t numRegistered = m_registered.size();
        for (size_t i = 0; i < numRegistered; ++i)
        {
            delete m_registered[i];
        }
        m_registered.clear();
    }

    AZ::Outcome<size_t> MotionDataFactory::FindRegisteredIndexByTypeId(const AZ::TypeId& typeId) const
    {
        auto findResult = AZStd::find_if(
            m_registered.begin(),
            m_registered.end(),
            [&](MotionData* data)
            {
                return (typeId == data->RTTI_GetType());
            } );

        if (findResult == m_registered.end())
        {
            return AZ::Failure();
        }

        const size_t index = findResult - m_registered.begin();
        return AZ::Success(index);
    }

    AZ::Outcome<size_t> MotionDataFactory::FindRegisteredIndexByTypeName(const AZStd::string& typeName) const
    {
        auto findResult = AZStd::find_if(
            m_registered.begin(),
            m_registered.end(),
            [&](MotionData* data)
            {
                return (typeName == data->RTTI_GetTypeName());
            } );

        if (findResult == m_registered.end())
        {
            return AZ::Failure();
        }

        const size_t index = findResult - m_registered.begin();
        return AZ::Success(index);
    }

    bool MotionDataFactory::IsRegisteredTypeId(const AZ::TypeId& typeId) const
    {
        return FindRegisteredIndexByTypeId(typeId).IsSuccess();
    }

    bool MotionDataFactory::IsRegisteredTypeName(const AZStd::string& typeName) const
    {
        return FindRegisteredIndexByTypeName(typeName).IsSuccess();
    }

    void MotionDataFactory::Register(MotionData* motionData)
    {
        AZ_Assert(motionData, "Expected motionData to not be a nullptr.");
        if (!motionData)
        {
            return;
        }
        const bool isAlreadyRegistered = IsRegisteredTypeId(motionData->RTTI_Type());
        AZ_Assert(!isAlreadyRegistered, "We already registered the motion data type '%s'.", motionData->RTTI_GetTypeName());
        if (!isAlreadyRegistered)
        {
            m_registered.emplace_back(motionData);
        }
    }

    MotionData* MotionDataFactory::Create(const AZ::TypeId& typeId) const
    {
        const AZ::Outcome<size_t> findResult = FindRegisteredIndexByTypeId(typeId);
        return findResult.IsSuccess() ? m_registered[findResult.GetValue()]->CreateNew() : nullptr;
    }

    size_t MotionDataFactory::GetNumRegistered() const
    {
        return m_registered.size();
    }

    const MotionData* MotionDataFactory::GetRegistered(size_t index) const
    {
        return m_registered[index];
    }

    void MotionDataFactory::Unregister(const AZ::TypeId& typeId)
    {
        const AZ::Outcome<size_t> index = FindRegisteredIndexByTypeId(typeId);
        if (!index.IsSuccess())
        {
            AZ_Warning("EMotionFX", false, "MotionDataFactory doesn't have any registered type '%s' to unregister.", typeId.ToString<AZStd::string>().c_str());
            return;
        }

        delete m_registered[index.GetValue()];
        m_registered.erase(m_registered.begin() + index.GetValue());
    }
}

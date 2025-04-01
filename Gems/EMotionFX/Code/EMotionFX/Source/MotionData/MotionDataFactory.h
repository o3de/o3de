/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>

namespace EMotionFX
{
    class MotionData;

    class EMFX_API MotionDataFactory
    {
    public:
        AZ_CLASS_ALLOCATOR(MotionDataFactory, MotionAllocator)
        AZ_RTTI(MotionDataFactory, "{9A8C3075-788F-4BA0-A60E-ABC13E753C65}")

        MotionDataFactory() = default;
        MotionDataFactory(const MotionDataFactory&) = delete;
        virtual ~MotionDataFactory();

        MotionDataFactory& operator=(const MotionDataFactory&) = delete;
        MotionDataFactory& operator=(const MotionDataFactory&&) = delete;

        void Init(); // Registers default internal MotionData types.
        void Clear();
        void Register(MotionData* motionData);
        void Unregister(const AZ::TypeId& typeId);
        MotionData* Create(const AZ::TypeId& typeId) const;

        size_t GetNumRegistered() const;
        const MotionData* GetRegistered(size_t index) const;
        AZ::Outcome<size_t> FindRegisteredIndexByTypeId(const AZ::TypeId& typeId) const;
        AZ::Outcome<size_t> FindRegisteredIndexByTypeName(const AZStd::string& typeName) const;
        bool IsRegisteredTypeId(const AZ::TypeId& typeId) const;
        bool IsRegisteredTypeName(const AZStd::string& typeName) const;

    private:
        AZStd::vector<MotionData*> m_registered;
    };
} // namespace EMotionFX

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/std/utils.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <ScriptCanvas/Data/Traits.h>

namespace ScriptCanvas
{
    struct TypeProperties
    {
        bool m_isTransient = false;
    };

    namespace Data
    {
        struct TypeErasedTraits;
    }

    struct DataRegistry final
    {
        AZ_TYPE_INFO(DataRegistry, "{41049FA8-EA56-401F-9720-6FE9028A1C01}");
        AZ_CLASS_ALLOCATOR(DataRegistry, AZ::SystemAllocator);

        enum class Createability
        {
            None,
            SlotAndVariable,
            SlotOnly,
        };
        void RegisterType(const AZ::TypeId& typeId, TypeProperties typeProperties, Createability registration);
        void UnregisterType(const AZ::TypeId& typeId);

        bool IsUseableInSlot(const AZ::TypeId& typeId) const;
        bool IsUseableInSlot(const Data::Type& type) const;

        AZStd::unordered_map<Data::eType, Data::TypeErasedTraits> m_typeIdTraitMap; // Creates a mapping of the Data::eType TypeId to the trait structure
        AZStd::unordered_map<Data::Type, TypeProperties> m_creatableTypes;
        AZStd::unordered_map<Data::Type, TypeProperties> m_slottableTypes;
    };

    void InitDataRegistry();
    void ResetDataRegistry();
    extern AZ::EnvironmentVariable<DataRegistry> GetDataRegistry();

    static constexpr const char* s_dataRegistryName = "ScriptCanvasDataRegistry";
}

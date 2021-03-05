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

#include <AzCore/Module/Environment.h>
#include <ScriptCanvas/Data/Traits.h>
#include <AzCore/std/utils.h>
#include <AzCore/RTTI/ReflectContext.h>

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
        AZ_CLASS_ALLOCATOR(DataRegistry, AZ::SystemAllocator, 0);

        void RegisterType(const AZ::TypeId& typeId, TypeProperties typeProperties);
        void UnregisterType(const AZ::TypeId& typeId);

        AZStd::unordered_map<Data::eType, Data::TypeErasedTraits> m_typeIdTraitMap; // Creates a mapping of the Data::eType TypeId to the trait structure
        AZStd::unordered_map<Data::Type, TypeProperties> m_creatableTypes;
    };

    void InitDataRegistry();
    void ResetDataRegistry();
    extern AZ::EnvironmentVariable<DataRegistry> GetDataRegistry();
    static const char* s_dataRegistryName = "ScriptCanvasDataRegistry";
}

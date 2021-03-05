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

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include <ScriptCanvas/Deprecated/VariableDatum.h>

namespace ScriptCanvas
{
    namespace Deprecated
    {
        //! Maintains the data type and variableId associated with a data slot
        //! This structure is used as value in a map where the key is the slot being associated with a Data::Type
        struct VariableInfo
        {
            AZ_TYPE_INFO(VariableInfo, "{57DEBC6B-8708-454B-96DC-0A34D1835709}");
            AZ_CLASS_ALLOCATOR(VariableInfo, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            VariableInfo() = default;
            VariableInfo(const VariableId& nodeOwnedVarId);
            VariableInfo(const Data::Type& dataType);

            VariableId m_currentVariableId; // Variable Id of VariableDatum to use when accessing the associated slot data input
            VariableId m_ownedVariableId; // Variable Id of VariableDatum which is managed by this node and associated with the slot
            Data::Type m_dataType;
        };        
    }
}
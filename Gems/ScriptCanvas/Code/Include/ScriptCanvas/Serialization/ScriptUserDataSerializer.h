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

#include "GenericDependantPairSerializer.h"

namespace AZ
{
    class ScriptUserDataSerializer
        : public GenericDependantPairSerializer<ScriptUserDataSerializer>
    {
    public:
        AZ_RTTI(ScriptUserDataSerializer, "{7E5FC193-8CDB-4251-A68B-F337027381DF}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    private:
        friend class GenericDependantPairSerializer<ScriptUserDataSerializer>;

        // The key name of the first component of the pair (when serialized as an object)
        static const char* IndexMemberName();

        // The key name of the second component of the pair (when serialized as an object)
        static const char* ValueMemberName();

        // A human readable type name for the type being handled
        static const char* PrettyTypeName();

        // Defines the type of the first component
        using IndexMemberType = AZ::Uuid;

        // Given a value for the index member, retrieves the corresponding data type
        // The serialized representation may not always have an explicit value for the index, so index type is optional
        static AZStd::optional<TypeOption> GetIndexTypeFromIndex(IDataContainer&, const AZStd::optional<IndexMemberType>&);

        // Given the type of the stored second component, retrieves the value of the correspond index
        static AZStd::optional<IndexMemberType> GetIndexFromIndexType(IDataContainer&, const TypeOption&);
    };
}

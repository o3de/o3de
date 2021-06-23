/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Budget.h>

#include <ScriptCanvas/Data/DataTraitBase.h>
#include <ScriptCanvas/Data/DataTypeUtils.h>

AZ_DECLARE_BUDGET(ScriptCanvas);

namespace AZ
{
    class ReflectContext;
    struct BehaviorParameter;
}

namespace ScriptCanvas
{
    namespace Data
    {
        template<typename T>
        AZ_INLINE Type FromAZType()
        {
            return FromAZType(azrtti_typeid<T>());
        }

        // returns the most raw name for the type
        const char* GetBehaviorContextName(const AZ::Uuid& azType);
        const char* GetBehaviorContextName(const Type& type);

        // returns a possibly prettier name for the type
        AZStd::string GetBehaviorClassName(const AZ::Uuid& typeID);

        // returns a possibly prettier name for the type
        AZStd::string GetName(const Type& type);
        const Type GetBehaviorParameterDataType(const AZ::BehaviorParameter& parameter);

        AZStd::vector<AZ::Uuid> GetContainedTypes(const AZ::Uuid& type);
        AZStd::vector<Type> GetContainedTypes(const Type& type);
        AZStd::pair<AZ::Uuid, AZ::Uuid> GetOutcomeTypes(const AZ::Uuid& type);
        AZStd::pair<Type, Type> GetOutcomeTypes(const Type& type);
    }
} 

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::Data::Type>
    {
        size_t operator()(const ScriptCanvas::Data::Type& ref) const
        {
            size_t seed = 0U;
            hash_combine(seed, ref.GetType());
            hash_combine(seed, ScriptCanvas::Data::ToAZType(ref));
            return seed;
        }
    };
}

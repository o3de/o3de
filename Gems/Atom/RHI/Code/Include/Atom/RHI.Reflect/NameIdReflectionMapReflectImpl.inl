/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    namespace RHI
    {
        template <typename IndexType>
        void NameIdReflectionMap<IndexType>::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<typename NameIdReflectionMap<IndexType>::ReflectionPair>()
                    ->Version(2)
                    ->Field("Name", &ReflectionPair::m_name)
                    ->Field("Index", &ReflectionPair::m_index);

                serializeContext->Class<NameIdReflectionMap<IndexType>>()
                    ->Version(0)
                    ->template EventHandler<typename NameIdReflectionMap<IndexType>::NameIdReflectionMapSerializationEvents>()
                    ->Field("ReflectionMap", &NameIdReflectionMap<IndexType>::m_reflectionMap);
            }
        }
    }
}

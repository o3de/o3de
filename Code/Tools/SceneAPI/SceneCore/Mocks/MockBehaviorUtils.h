/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Script/ScriptContextAttributes.h>

namespace UnitTest
{
    void RemoveAttributePair(AZ::AttributeArray& attributes, AZ::AttributeId attributeId)
    {
        using namespace AZ::Script;
        auto attributePair = AZStd::find_if(attributes.begin(), attributes.end(), [attributeId](const AZ::AttributePair& pair)
        {
            return pair.first == attributeId;
        });
        if (attributePair != attributes.end())
        {
            // clean up the old attribute value
            if (attributePair->second)
            {
                delete attributePair->second;
            }
            attributes.erase(attributePair);
        }
    }

    void ScopeForUnitTest(AZ::AttributeArray& attributes)
    {
        RemoveAttributePair(attributes, AZ::Script::Attributes::Scope);
        attributes.push_back(AZStd::make_pair(
            AZ::Script::Attributes::Scope,
            aznew AZ::AttributeData<AZ::Script::Attributes::ScopeFlags>(AZ::Script::Attributes::ScopeFlags::Common)));
    }

    void ApplyStorageForUnitTest(AZ::AttributeArray& attributes)
    {
        RemoveAttributePair(attributes, AZ::Script::Attributes::Storage);
        attributes.push_back(AZStd::make_pair(
            AZ::Script::Attributes::Storage,
            aznew AZ::AttributeData<AZ::Script::Attributes::StorageType>(AZ::Script::Attributes::StorageType::Value)));
    }
}

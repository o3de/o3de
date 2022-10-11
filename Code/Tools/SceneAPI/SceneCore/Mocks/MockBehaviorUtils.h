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
    void ScopeForUnitTest(AZ::AttributeArray& attributes)
    {
        using namespace AZ::Script;
        attributes.erase(AZStd::remove_if(attributes.begin(), attributes.end(), [](const AZ::AttributePair& pair)
            {
                return pair.first == Attributes::Scope;
            }));
        auto* attributeData = aznew AZ::AttributeData<Attributes::ScopeFlags>(Attributes::ScopeFlags::Common);
        attributes.push_back(AZStd::make_pair(Attributes::Scope, attributeData));
    }
}

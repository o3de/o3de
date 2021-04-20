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

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    class EntityOutlinerWidgetInterface
    {
    public:
        AZ_RTTI(EntityOutlinerWidgetInterface, "{30C0F252-EC84-4196-BF59-EB9E73B8ADCB}");

        virtual void SetUpdatesEnabled(bool enable) = 0;
        virtual void ExpandEntityChildren(AZ::EntityId entityId) = 0;
    };

} // namespace AzToolsFramework


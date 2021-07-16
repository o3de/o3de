/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzFramework
{
    namespace Components
    {
        struct DeprecatedInfo
        {
            bool m_hideComponent = false;
            AZStd::string m_deprecationString;
        };
        using DeprecatedComponentsList = AZStd::unordered_map<AZ::Uuid, DeprecatedInfo>;

        //! Use this bus to get a list of the Uuid's of any class that is deprecated.
        //! The list contains the Uuid's of all the deprecated classes, 
        //! a flag that the class should be hidden in the add component menu,
        //! some text to be appended to the classes name in the add component menu.
        //! List will be appended to by each handler of the bus.
        class DeprecatedComponents
        : public AZ::EBusTraits
        {
        public:
            virtual void EnumerateDeprecatedComponents(DeprecatedComponentsList& list) const = 0;
        };
        using DeprecatedComponentsRequestBus = AZ::EBus<DeprecatedComponents>;
    } // namespace Components
} // namespace AzFramework

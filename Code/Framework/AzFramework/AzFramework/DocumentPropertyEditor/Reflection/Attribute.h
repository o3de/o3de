/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomValue.h>
#include <AzCore/std/functional.h>
#include <AzCore/Name/Name.h>

namespace AZ::Reflection
{
    using AttributeDataType = Dom::Value;

    struct IAttributes
    {
        using IterationCallback =
            AZStd::function<void(Name group, Name name, const AttributeDataType& attribute)>;

        virtual const AttributeDataType* Find(Name name) const = 0;
        virtual const AttributeDataType* Find(Name group, Name name) const = 0;
        virtual void ListAttributes(const IterationCallback& callback) const = 0;
    };
}

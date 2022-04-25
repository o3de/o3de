/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/RoutingAdapter.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::DocumentPropertyEditor
{
    class ReflectionAdapter : public RoutingAdapter
    {
    public:
        ReflectionAdapter() = default;
        ReflectionAdapter(void* instance, const AZ::TypeId typeId);

        void SetValue(void* instance, const AZ::TypeId typeId);

        Dom::Value GetContents() const override;

    private:
        void* m_instance = nullptr;
        AZ::TypeId m_typeId;

        friend struct ReflectionAdapterReflectionImpl;
    };
}

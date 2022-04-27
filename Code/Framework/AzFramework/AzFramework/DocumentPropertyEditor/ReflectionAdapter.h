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
    //! ReflectionAdapter turns an in-memory instance of an object backed by
    //! the AZ Reflection system (via SerializeContext & EditContext) and creates
    //! a property grid that supports editing its members in a manner outlined by
    //! its reflection data.
    class ReflectionAdapter : public RoutingAdapter
    {
    public:
        //! Creates an uninitialized (empty) ReflectionAdapter.
        ReflectionAdapter() = default;
        //! Creates a ReflectionAdapter with a contents comrpised of the reflected data of
        //! the specified instance.
        //! \see SetValue
        ReflectionAdapter(void* instance, const AZ::TypeId typeId);

        //! Sets the instance to reflect. If typeId is a type registered to SerializeContext,
        //! this adapter will produce a property grid based on its contents.
        void SetValue(void* instance, const AZ::TypeId typeId);

        Dom::Value GetContents() const override;

    private:
        void* m_instance = nullptr;
        AZ::TypeId m_typeId;

        friend struct ReflectionAdapterReflectionImpl;
    };
}

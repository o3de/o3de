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
    struct ReflectionAdapterReflectionImpl;

    //! ReflectionAdapter turns an in-memory instance of an object backed by
    //! the AZ Reflection system (via SerializeContext & EditContext) and creates
    //! a property grid that supports editing its members in a manner outlined by
    //! its reflection data.
    class ReflectionAdapter : public RoutingAdapter
    {
    public:
        //! Creates an uninitialized (empty) ReflectionAdapter.
        ReflectionAdapter();
        //! Creates a ReflectionAdapter with a contents comrpised of the reflected data of
        //! the specified instance.
        //! \see SetValue
        ReflectionAdapter(void* instance, AZ::TypeId typeId);
        ~ReflectionAdapter();

        //! Sets the instance to reflect. If typeId is a type registered to SerializeContext,
        //! this adapter will produce a property grid based on its contents.
        void SetValue(void* instance, AZ::TypeId typeId);

        Dom::Value GetContents() const override;

        void OnContentsChanged(const Dom::Path& path, const Dom::Value& value);

    private:
        void* m_instance = nullptr;
        AZ::TypeId m_typeId = AZ::TypeId::CreateNull();

        mutable AZStd::unique_ptr<ReflectionAdapterReflectionImpl> m_impl;

        friend struct ReflectionAdapterReflectionImpl;
    };
}

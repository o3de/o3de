/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/Attribute.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/Visitor.h>

namespace AZ::Reflection
{
    //! These synthetic attributes are injected into legacy reflection data to give additional context
    //! for use in the DocumentPropertyEditor.
    namespace DescriptorAttributes
    {
        //! The UIHandler (or PropertyEditor Type) this property should use.
        //! Type: String
        extern const Name Handler;
        //! If specified, this property should have a label with the specified text.
        //! Type: String
        extern const Name Label;
        //! Specifies the JSON path to where this property would be located in a JSON serialized instance,
        //! relative to the instance parameter to a Visit call.
        //! Type: String (can be parsed by AZ::Dom::Path)
        extern const Name SerializedPath;
    } // namespace DescriptorAttributes

    void VisitLegacyInMemoryInstance(
        IRead* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext = nullptr);
    void VisitLegacyInMemoryInstance(
        IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext = nullptr);
    // TODO: Currently we only support reflecting in-memory instances, but we'll also need to be able to reflect JSON-serialized
    // instances to interface with the Prefab System.
    // void VisitLegacyJsonSerializedInstance(IRead* visitor, Dom::Value instance, const AZ::TypeId& typeId);
} // namespace AZ::Reflection

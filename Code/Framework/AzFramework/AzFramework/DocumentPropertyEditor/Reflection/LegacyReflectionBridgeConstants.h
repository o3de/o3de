/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>

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
        //! The serialize context container interface for a container row.
        //! Type: AZ::SerializeContext::IDataContainer (marshalled as a void* because it's not reflected)
        extern const Name Container;
        //! The serialize context container interface for a container element's parent.
        //! Type: AZ::SerializeContext::IDataContainer (marshalled as a void* because it's not reflected)
        extern const Name ParentContainer;
        //! The container instance pointer for a container element row.
        //! Type: void*
        extern const Name ParentContainerInstance;
        //! Boolean flag indicating whether the owning container of an element can be modified
        extern const Name ParentContainerCanBeModified;
        //! If specified, an override for the instance of the element referenced in a container operation.
        //! Type: void*
        extern const Name ContainerElementOverride;
        //! the index of the element, if it is a member of a sequenced container
        //! Type: size_t
        extern const Name ContainerIndex;
        extern const Name ParentInstance;
        extern const Name ParentClassData;
    } // namespace DescriptorAttributes
} // namespace AZ::Reflection

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomVisitor.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::Dom
{
    //! Backends are registered centrally and used to transition DOM formats to and from a textual format.
    class Backend
    {
    public:
        virtual ~Backend() = default;

        //! Attempt to read this format from the given buffer into the target Visitor.
        virtual Visitor::Result ReadFromBuffer(
            const char* buffer, size_t size, AZ::Dom::Lifetime lifetime, Visitor& visitor) = 0;
        //! Attempt to read this format from a mutable string into the target Visitor. This enables some backends to
        //! parse without making additional string allocations.
        //! This string must be null terminated.
        //! This string may be modified and read in place without being copied, so when calling this please ensure that:
        //! - The string won't be deallocated until the visitor no longer needs the values and
        //! - The string is safe to modify in place.
        //! The base implementation simply calls ReadFromBuffer.
        virtual Visitor::Result ReadFromBufferInPlace(char* buffer, AZStd::optional<size_t> size, Visitor& visitor);

        //! A callback that accepts a Visitor, making DOM calls to inform the serializer, and returns an
        //! aggregate error code to indicate whether or not the operation succeeded.
        using WriteCallback = AZStd::function<Visitor::Result(Visitor&)>;
        //! Attempt to write a value to the specified string using a write callback.
        virtual Visitor::Result WriteToBuffer(AZStd::string& buffer, WriteCallback callback) = 0;
    };
} // namespace AZ::Dom

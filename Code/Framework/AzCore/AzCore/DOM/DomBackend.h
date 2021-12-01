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
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::Dom
{
    //! Backends are registered centrally and used to transition DOM formats to and from a textual format.
    class Backend
    {
    public:
        virtual ~Backend() = default;

        //! Attempt to read this format from the given stream into the target Visitor.
        //! The base implementation reads the stream into memory and calls ReadFromString.
        virtual Visitor::Result ReadFromStream(
            AZ::IO::GenericStream* stream, Visitor& visitor, size_t maxSize = AZStd::numeric_limits<size_t>::max());
        //! Attempt to read this format from a mutable string into the target Visitor. This enables some backends to
        //! parse without making additional string allocations.
        //! This string may be modified and read in place without being copied, so when calling this please ensure that:
        //! - The string won't be deallocated until the visitor no longer needs the values and
        //! - The string is safe to modify in place.
        //! The base implementation simply calls ReadFromString.
        virtual Visitor::Result ReadFromStringInPlace(AZStd::string& buffer, Visitor& visitor);
        //! Attempt to read this format from an immutable buffer in memory into the target Visitor.
        virtual Visitor::Result ReadFromString(AZStd::string_view buffer, Lifetime lifetime, Visitor& visitor) = 0;

        //! Acquire a visitor interface for writing to the target output file.
        virtual AZStd::unique_ptr<Visitor> CreateStreamWriter(AZ::IO::GenericStream& stream) = 0;
        //! A callback that accepts a Visitor, making DOM calls to inform the serializer, and returns an
        //! aggregate error code to indicate whether or not the operation succeeded.
        using WriteCallback = AZStd::function<Visitor::Result(Visitor&)>;
        //! Attempt to write a value to a stream using a write callback.
        Visitor::Result WriteToStream(AZ::IO::GenericStream& stream, WriteCallback callback);
        //! Attempt to write a value to a string using a write callback.
        Visitor::Result WriteToString(AZStd::string& buffer, WriteCallback callback);
    };
} // namespace AZ::Dom

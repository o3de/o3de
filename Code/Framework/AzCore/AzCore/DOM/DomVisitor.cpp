/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomVisitor.h>

namespace AZ::DOM
{
    const char* VisitorError::CodeToString(VisitorErrorCode code)
    {
        switch (code)
        {
        case VisitorErrorCode::UnsupportedOperation:
            return "operation not supported";
        case VisitorErrorCode::InvalidData:
            return "invalid data specified";
        case VisitorErrorCode::InternalError:
            return "internal error";
        default:
            return "unknown error";
        }
    }

    VisitorError::VisitorError(VisitorErrorCode code)
        : m_code(code)
    {
    }

    VisitorError::VisitorError(VisitorErrorCode code, AZStd::string additionalInfo)
        : m_code(code)
        , m_additionalInfo(additionalInfo)
    {
    }

    VisitorErrorCode VisitorError::GetCode() const
    {
        return m_code;
    }

    const AZStd::string& VisitorError::GetAdditionalInfo() const
    {
        return m_additionalInfo;
    }

    AZStd::string VisitorError::FormatVisitorErrorMessage() const
    {
        if (m_additionalInfo.empty())
        {
            return AZStd::string::format("VisitorError: %s.", CodeToString(m_code));
        }
        return AZStd::string::format("VisitorError: %s. %s.", CodeToString(m_code), m_additionalInfo.c_str());
    }

    Visitor::Result Visitor::VisitorFailure(VisitorErrorCode code)
    {
        return AZ::Failure(VisitorError(code));
    }

    Visitor::Result Visitor::VisitorFailure(VisitorErrorCode code, AZStd::string additionalInfo)
    {
        return AZ::Failure(VisitorError(code, additionalInfo));
    }

    Visitor::Result Visitor::VisitorFailure(VisitorError error)
    {
        return AZ::Failure(error);
    }

    Visitor::Result Visitor::VisitorSuccess()
    {
        return AZ::Success();
    }

    Visitor::Result Visitor::Null()
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::Bool([[maybe_unused]] bool value)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::Int64([[maybe_unused]] AZ::s64 value)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::Uint64([[maybe_unused]] AZ::u64 value)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::Double([[maybe_unused]] double value)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::String([[maybe_unused]] AZStd::string_view value, [[maybe_unused]] StorageSemantics storageSemantics)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::OpaqueValue([[maybe_unused]] const OpaqueType& value, [[maybe_unused]] StorageSemantics storageSemantics)
    {
        return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Opaque values are not supported by this visitor");
    }

    Visitor::Result Visitor::RawValue([[maybe_unused]]AZStd::string_view value, [[maybe_unused]]StorageSemantics storageSemantics)
    {
        return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Raw values are not supported by this visitor");
    }

    Visitor::Result Visitor::StartObject()
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::EndObject([[maybe_unused]] AZ::u64 attributeCount)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::Key([[maybe_unused]] AZ::Name key)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::RawKey(AZStd::string_view key, [[maybe_unused]]StorageSemantics storageSemantics)
    {
        return Key(AZ::Name(key));
    }

    Visitor::Result Visitor::StartArray()
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::EndArray([[maybe_unused]] AZ::u64 elementCount)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::StartNode([[maybe_unused]] AZ::Name name)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::RawStartNode(AZStd::string_view name, [[maybe_unused]]StorageSemantics storageSemantics)
    {
        return StartNode(AZ::Name(name));
    }

    Visitor::Result Visitor::EndNode([[maybe_unused]] AZ::u64 attributeCount, [[maybe_unused]] AZ::u64 elementCount)
    {
        return VisitorSuccess();
    }

    VisitorFlags Visitor::GetVisitorFlags() const
    {
        return VisitorFlags::SupportsRawKeys | VisitorFlags::SupportsArrays | VisitorFlags::SupportsObjects | VisitorFlags::SupportsNodes;
    }

    bool Visitor::SupportsRawValues() const
    {
        return (GetVisitorFlags() & VisitorFlags::SupportsRawValues) != VisitorFlags::Null;
    }

    bool Visitor::SupportsRawKeys() const
    {
        return (GetVisitorFlags() & VisitorFlags::SupportsRawKeys) != VisitorFlags::Null;
    }

    bool Visitor::SupportsObjects() const
    {
        return (GetVisitorFlags() & VisitorFlags::SupportsObjects) != VisitorFlags::Null;
    }

    bool Visitor::SupportsArrays() const
    {
        return (GetVisitorFlags() & VisitorFlags::SupportsArrays) != VisitorFlags::Null;
    }

    bool Visitor::SupportsNodes() const
    {
        return (GetVisitorFlags() & VisitorFlags::SupportsNodes) != VisitorFlags::Null;
    }

    bool Visitor::SupportsOpaqueValues() const
    {
        return (GetVisitorFlags() & VisitorFlags::SupportsOpaqueValues) != VisitorFlags::Null;
    }
} // namespace AZ::DOM

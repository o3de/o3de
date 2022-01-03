/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomVisitor.h>

namespace AZ::Dom
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
        , m_additionalInfo(AZStd::move(additionalInfo))
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
        return AZ::Failure(VisitorError(code, AZStd::move(additionalInfo)));
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

    Visitor::Result Visitor::String([[maybe_unused]] AZStd::string_view value, [[maybe_unused]] Lifetime lifetime)
    {
        return VisitorSuccess();
    }

    Visitor::Result Visitor::OpaqueValue([[maybe_unused]] const OpaqueType& value, [[maybe_unused]] Lifetime lifetime)
    {
        if (!SupportsOpaqueValues())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Opaque values are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::RawValue([[maybe_unused]] AZStd::string_view value, [[maybe_unused]] Lifetime lifetime)
    {
        if (!SupportsRawValues())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Raw values are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::StartObject()
    {
        if (!SupportsObjects())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Objects are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::EndObject([[maybe_unused]] AZ::u64 attributeCount)
    {
        if (!SupportsObjects())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Objects are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::Key([[maybe_unused]] AZ::Name key)
    {
        if (!SupportsObjects() && !SupportsNodes())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Keys are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::RawKey(AZStd::string_view key, [[maybe_unused]] Lifetime lifetime)
    {
        if (!SupportsRawKeys())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Raw keys are not supported by this visitor");
        }
        return Key(AZ::Name(key));
    }

    Visitor::Result Visitor::StartArray()
    {
        if (!SupportsArrays())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Arrays are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::EndArray([[maybe_unused]] AZ::u64 elementCount)
    {
        if (!SupportsArrays())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Arrays are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::StartNode([[maybe_unused]] AZ::Name name)
    {
        if (!SupportsNodes())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Nodes are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    Visitor::Result Visitor::RawStartNode(AZStd::string_view name, [[maybe_unused]] Lifetime lifetime)
    {
        return StartNode(AZ::Name(name));
    }

    Visitor::Result Visitor::EndNode([[maybe_unused]] AZ::u64 attributeCount, [[maybe_unused]] AZ::u64 elementCount)
    {
        if (!SupportsNodes())
        {
            return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Nodes are not supported by this visitor");
        }
        return VisitorSuccess();
    }

    VisitorFlags Visitor::GetVisitorFlags() const
    {
        // By default support raw keys (promoting them to AZ::Name) and support Array / Object / Node
        // We leave Opaque type support and Raw Values to more specialized, implementation-specific cases
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
} // namespace AZ::Dom

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DOMVisitor.h>

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
        default:
            return "internal error";
        }
    }

    VisitorError::VisitorError(VisitorErrorCode code)
        : m_code(code)
    {
    }

    VisitorError::VisitorError(VisitorErrorCode code, AZStd::string_view additionalInfo)
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

    VisitorInterface::Result VisitorInterface::VisitorFailure(VisitorErrorCode code)
    {
        return AZ::Failure(VisitorError(code));
    }

    VisitorInterface::Result VisitorInterface::VisitorFailure(VisitorErrorCode code, AZStd::string_view additionalInfo)
    {
        return AZ::Failure(VisitorError(code, additionalInfo));
    }

    VisitorInterface::Result VisitorInterface::VisitorFailure(VisitorError error)
    {
        return AZ::Failure(error);
    }

    VisitorInterface::Result VisitorInterface::VisitorSuccess()
    {
        return AZ::Success();
    }

    VisitorInterface::Result VisitorInterface::Null()
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::Bool([[maybe_unused]] bool value)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::Int64([[maybe_unused]] AZ::s64 value)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::Uint64([[maybe_unused]] AZ::u64 value)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::Double([[maybe_unused]] double value)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::String(
        [[maybe_unused]] AZStd::string_view value, [[maybe_unused]] StorageSemantics storageSemantics)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::OpaqueValue(
        [[maybe_unused]] const OpaqueType& value, [[maybe_unused]] StorageSemantics storageSemantics)
    {
        return VisitorFailure(VisitorErrorCode::UnsupportedOperation, "Opaque values are not supported by this visitor");
    }

    VisitorInterface::Result VisitorInterface::StartObject()
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::EndObject([[maybe_unused]] AZ::u64 attributeCount)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::Key([[maybe_unused]] AZ::Name)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::StartArray()
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::EndArray([[maybe_unused]] AZ::u64 elementCount)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::StartNode([[maybe_unused]] AZ::Name name)
    {
        return VisitorSuccess();
    }

    VisitorInterface::Result VisitorInterface::EndNode([[maybe_unused]] AZ::u64 attributeCount, [[maybe_unused]] AZ::u64 elementCount)
    {
        return VisitorSuccess();
    }
} // namespace AZ::DOM

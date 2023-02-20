/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::IO::Internal
{
    // Helper function for url quoting a relative path string
    // Only implemented in Path.cpp for AZStd::string and AZStd::fixed_string<MaxPathLength>
    template <class StringType>
    StringType AsUri(const PathView& pathView) noexcept;
}


// Explicit instantations of our support Path classes
namespace AZ::IO
{
        // string conversion
    AZStd::string PathView::String() const
    {
        return AZStd::string(m_path);
    }

    // as_posix
    AZStd::string PathView::StringAsPosix() const
    {
        AZStd::string resultPath(m_path);
        AZStd::replace(resultPath.begin(), resultPath.end(), AZ::IO::WindowsPathSeparator, AZ::IO::PosixPathSeparator);
        return resultPath;
    }

    // as_uri
    AZStd::string PathView::StringAsUri() const
    {
        return Internal::AsUri<AZStd::string>(*this);
    }

    // as_uri
    // Encodes the path suitable for using in a URI
    AZStd::fixed_string<MaxPathLength> PathView::AsUri() const noexcept
    {
        return FixedMaxPathStringAsUri();
    }

    AZStd::fixed_string<MaxPathLength> PathView::FixedMaxPathStringAsUri() const noexcept
    {
        return Internal::AsUri<AZStd::fixed_string<MaxPathLength>>(*this);
    }


    // as_uri
    // Encodes the path suitable for using in a URI
    template <typename StringType>
    auto BasicPath<StringType>::AsUri() const -> string_type
    {
        return Internal::AsUri<string_type>(*this);
    }
    template <typename StringType>
    AZStd::string BasicPath<StringType>::StringAsUri() const
    {
        return Internal::AsUri<AZStd::string>(*this);
    }

    template <typename StringType>
    AZStd::fixed_string<MaxPathLength> BasicPath<StringType>::FixedMaxPathStringAsUri() const noexcept
    {
        return Internal::AsUri<AZStd::fixed_string<MaxPathLength>>(*this);
    }
    
    // Class template instantations
    template class BasicPath<AZStd::string>;
    template class BasicPath<FixedMaxPathString>;
    template class PathIterator<const PathView>;
    template class PathIterator<const Path>;
    template class PathIterator<const FixedMaxPath>;

    // Swap function instantiations
    template void swap<AZStd::string>(Path& lhs, Path& rhs) noexcept;
    template void swap<FixedMaxPathString>(FixedMaxPath& lhs, FixedMaxPath& rhs) noexcept;

    // Hash function instantiations
    template size_t hash_value<AZStd::string>(const Path& pathToHash);
    template size_t hash_value<FixedMaxPathString>(const FixedMaxPath& pathToHash);

    // Append operator instantiations
    template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs, const PathView& rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, const PathView& rhs);
    template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs, AZStd::string_view rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs, AZStd::string_view rhs);
    template BasicPath<AZStd::string> operator/<AZStd::string>(const BasicPath<AZStd::string>& lhs,
        const typename BasicPath<AZStd::string>::value_type* rhs);
    template BasicPath<FixedMaxPathString> operator/<FixedMaxPathString>(const BasicPath<FixedMaxPathString>& lhs,
        const typename BasicPath<FixedMaxPathString>::value_type* rhs);

    // Iterator compare instantiations
    template bool operator==<const PathView>(const PathIterator<const PathView>& lhs,
        const PathIterator<const PathView>& rhs);
    template bool operator==<const Path>(const PathIterator<const Path>& lhs,
        const PathIterator<const Path>& rhs);
    template bool operator==<const FixedMaxPath>(const PathIterator<const FixedMaxPath>& lhs,
        const PathIterator<const FixedMaxPath>& rhs);
    template bool operator!=<const PathView>(const PathIterator<const PathView>& lhs,
        const PathIterator<const PathView>& rhs);
    template bool operator!=<const Path>(const PathIterator<const Path>& lhs,
        const PathIterator<const Path>& rhs);
    template bool operator!=<const FixedMaxPath>(const PathIterator<const FixedMaxPath>& lhs,
        const PathIterator<const FixedMaxPath>& rhs);
}

namespace AZ::IO::Internal
{
    //! Quotes a URL like path string
    //! Reference RFC 3986 for quoting: https://www.rfc-editor.org/rfc/rfc3986
    //! The unreserved characters are left as is https://www.rfc-editor.org/rfc/rfc3986#section-2.3
    //! and all reserved characters are URL encoded except for the '/' character as that is needed
    //! in a URI path
    static constexpr auto URIQuoteTable = []()
    {
        // Hex to Character macro for converting nibbles to characters
        constexpr AZStd::string_view HexToChar = "0123456789ABCDEF";
        static_assert(HexToChar.size() == 16);
        constexpr AZStd::string_view UnreservedCharacters = "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789"
            "-._~";
        static_assert(UnreservedCharacters.size() == 66);
        constexpr AZStd::string_view AllowedInPathCharacters = "/";

        AZStd::array<AZStd::fixed_string<3>, 256> percentQuoteTable{};
        for (size_t byteIndex = 0; byteIndex < percentQuoteTable.size(); ++byteIndex)
        {
            if (UnreservedCharacters.find_first_of(static_cast<char>(byteIndex)) != AZStd::string_view::npos
                || AllowedInPathCharacters.find_first_of(static_cast<char>(byteIndex)) != AZStd::string_view::npos)
            {
                percentQuoteTable[byteIndex] = static_cast<char>(byteIndex);
            }
            else
            {
                // Converts an ascii character such as '@' -> "%40"
                percentQuoteTable[byteIndex] += "%";
                percentQuoteTable[byteIndex] += HexToChar[(byteIndex / HexToChar.size()) % HexToChar.size()];
                percentQuoteTable[byteIndex] += HexToChar[byteIndex % HexToChar.size()];
            }
        }

        return percentQuoteTable;
    }();

    // Quick validation of some of quote conversions
    static_assert(URIQuoteTable['a'] == "a");
    static_assert(URIQuoteTable['-'] == "-");
    static_assert(URIQuoteTable[' '] == "%20");
    static_assert(URIQuoteTable['@'] == "%40");
    static_assert(URIQuoteTable[0] == "%00");
    // Allowed path character check
    static_assert(URIQuoteTable['/'] == "/");

    constexpr FixedMaxPathString UriQuote(AZStd::string_view path)
    {
        FixedMaxPathString quotedPath;
        for (char elem : path)
        {
            quotedPath += URIQuoteTable[elem];
        }

        return quotedPath;
    }

    template <class StringType>
    StringType AsUri(const PathView& pathView) noexcept
    {
        StringType resultUri;
        // The syntax for the "file:" URI scheme is documented at https://www.rfc-editor.org/rfc/rfc8089#page-3
        if (pathView.IsAbsolute())
        {
            const FixedMaxPath path = pathView.LexicallyNormal();
            if (path.HasRootName())
            {
                // Windows style path
                if (Internal::HasDrivePrefix(path.Native()))
                {
                    resultUri += "file:///";
                    resultUri += path.RootName().Native();
                    resultUri += AZ::IO::PosixPathSeparator;
                    resultUri += AZStd::string_view(UriQuote(path.RelativePath().FixedMaxPathStringAsPosix()));
                }
                else if (Internal::IsUncPath(path.Native(), path.PreferredSeparator()))
                {
                    resultUri += "file:";
                    resultUri += AZStd::string_view(UriQuote(path.FixedMaxPathStringAsPosix()));
                }
            }
            else
            {
                // Posix style path
                resultUri += "file://";
                resultUri += AZStd::string_view(UriQuote(path.FixedMaxPathStringAsPosix()));
            }
        }

        return resultUri;
    }

    template AZStd::string AsUri<AZStd::string>(const PathView& pathView) noexcept;
    template AZStd::fixed_string<MaxPathLength> AsUri<AZStd::fixed_string<MaxPathLength>>(const PathView& pathView) noexcept;
}

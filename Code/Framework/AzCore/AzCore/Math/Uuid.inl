/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Sha1.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/limits.h>

namespace AZ::UuidInternal
{
    constexpr AZStd::byte InvalidValue = AZStd::byte(255);

    // Lookup table to convert ascii values for 0-9, a-f, and A-F to hex values in the range 0-15.
    // Lambda expression populates the CharToHexDigit lookup table at compile time(it can't be invoked at runtime)
    // It is invoked immediately
    inline constexpr auto CharToHexDigit = []() constexpr
    {
        AZStd::array<AZStd::byte, 256> charToHexTable{};
        AZStd::fill(charToHexTable.begin(), charToHexTable.end(), InvalidValue);
        // Fill characters '0' - '9' with the byte range of 0 - 9
        for (size_t i = 0; i < 10; ++i)
        {
            charToHexTable['0' + i] = AZStd::byte(i);
        }
        // Fill characters 'A' - 'F' with the byte range of 10-15(for hex digits)
        for (size_t i = 0; i < 6; ++i)
        {
            charToHexTable['A' + i] = AZStd::byte(10 + i);
        }
        // Fill characters 'a' - 'f' with the byte range of 10-15(for hex digits)
        for (size_t i = 0; i < 6; ++i)
        {
            charToHexTable['a' + i] = AZStd::byte(10 + i);
        }

        return charToHexTable;
    }
    ();

    constexpr AZStd::byte GetValue(char c)
    {
        // Use a direct lookup table to convert from a valid ascii char to a 0-15 hex value
        return CharToHexDigit[c];
    }
}

namespace AZ
{
    constexpr Uuid::Uuid(AZStd::string_view uuidString)
    {
        operator=(CreateString(uuidString));
    }
    constexpr Uuid::Uuid(const char* string, size_t stringLength)
    {
        operator=(CreateString(string, stringLength));
    }

    constexpr Uuid Uuid::CreateNull()
    {
        return Uuid{};
    }

    constexpr Uuid Uuid::CreateInvalid()
    {
        return Uuid{ "{00000BAD-0BAD-0BAD-0BAD-000000000BAD}" };
    }

    constexpr Uuid Uuid::CreateString(const char* string, size_t stringLength)
    {
        return CreateString({ string, stringLength });
    }

    constexpr Uuid Uuid::CreateString(AZStd::string_view uuidString)
    {
        return CreateStringSkipWarnings(uuidString, false);
    }

    constexpr Uuid Uuid::CreateStringSkipWarnings(AZStd::string_view uuidString, [[maybe_unused]] bool skipWarnings)
    {
        if (uuidString.empty())
        {
            return {};
        }

        [[maybe_unused]] constexpr const char* InvalidFormatFormat = "Invalid UUID format %.*s (must be)"
            " {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx} (or without dashes and braces)";

        if (uuidString.size() < 32 || uuidString.size() > 38)
        {
            [[maybe_unused]] AZStd::string_view errorString = !uuidString.empty() ? uuidString : AZStd::string_view("null");
            AZ_Warning("Math", skipWarnings, InvalidFormatFormat, AZ_STRING_ARG(errorString));
            return {};
        }

        const char* current = uuidString.data();

        // check open brace
        char c = *current++;
        [[maybe_unused]] bool hasOpenBrace = false;
        if (c == '{')
        {
            c = *current++;
            hasOpenBrace = true;
        }
        bool hasDashes = false;
        Uuid id;
        for (int i = 0; i < 16; ++i)
        {
            if (i == 4)
            {
                hasDashes = (c == '-');
                if (hasDashes)
                {
                    c = *current++;
                }
            }

            if (hasDashes)
            {
                if (i == 6 || i == 8 || i == 10)
                {
                    if (c == '-')
                    {
                        c = *current++;
                    }
                    else
                    {
                        AZ_Warning("Math", skipWarnings, InvalidFormatFormat, AZ_STRING_ARG(uuidString));
                        return Uuid::CreateNull();
                    }
                }
            }

            id.m_data[i] = UuidInternal::GetValue(c);

            c = *current++;

            id.m_data[i] <<= 4;
            id.m_data[i] |= UuidInternal::GetValue(c);

            c = *current++;
        }

        // check close brace
        AZ_Warning("Math", !hasOpenBrace || skipWarnings || c == '}', InvalidFormatFormat,
            AZ_STRING_ARG(uuidString));

        return id;
    }


    constexpr Uuid Uuid::CreateStringPermissive(const char* uuidString, size_t stringLength, bool skipWarnings)
    {
        return CreateStringPermissive({ uuidString, stringLength }, skipWarnings);
    }

    constexpr Uuid Uuid::CreateStringPermissive(AZStd::string_view uuidString, bool skipWarnings)
    {
        constexpr size_t MaxPermissiveStringSize = 60;

        size_t newLength{ 0 };
        char createString[MaxPermissiveStringSize]{'\0'};

        // Loop until we get to the end of the string OR stop once we've accumulated a full UUID string worth of data
        for (size_t curPos = 0; curPos < uuidString.size() && newLength < ValidUuidStringLength; ++curPos)
        {
            char curChar = uuidString[curPos];
            switch (curChar)
            {
                // Bail early on expected no op characters
                case '{':
                {
                    break;
                }
                case '}':
                {
                    break;
                }
                case ' ':
                {
                    break;
                }
                case '-':
                {
                    break;
                }
                case 'X':
                    [[fallthrough]];
                case 'x':
                {
                    // If we get 0xdeadbeef ignore the 0x
                    if (curPos && uuidString[curPos - 1] == '0')
                    {
                        --newLength;
                    }
                    break;
                    // If for some reason it's xdeadbeef skip the X because this function is permissive.
                }
                default:
                {
                    if ((curChar >= '0' && curChar <= '9') || (curChar >= 'a' && curChar <= 'f') || (curChar >= 'A' && curChar <= 'F'))
                    {
                        createString[newLength++] = curChar;
                    }
                    else
                    {
                        // "Other" characters could also just be skipped here if we wanted to be even more permissive
                        if (!skipWarnings)
                        {
                            AZ_Warning("Math", false, "Unknown UUID character %c found at position %zu", curChar, curPos);
                        }
                        return {};
                    }
                }
            }
        }
        return CreateStringSkipWarnings({ createString, newLength }, skipWarnings);
    }

    // ToString conversion functions
    constexpr int Uuid::ToString(char* output, int outputSize, bool isBrackets, bool isDashes) const
    {
        int minSize = 32 + 1 /*1 is terminate*/;
        if (isDashes)
        {
            minSize += 4;
        }
        if (isBrackets)
        {
            minSize += 2;
        }

        if (outputSize < minSize)
        {
            return -minSize;
        }

        if (isBrackets)
        {
            *output++ = '{';
        }

        constexpr AZStd::string_view UuidChars = "0123456789ABCDEFabcdef";
        for (int i = 0; i < 16; ++i)
        {
            if (isDashes && (i == 4 || i == 6 || i == 8 || i == 10))
            {
                *output++ = '-';
            }

            AZStd::byte val = m_data[i];
            *output++ = UuidChars[static_cast<size_t>(val >> 4)];
            *output++ = UuidChars[static_cast<size_t>(val & AZStd::byte(0xF))];
        }

        if (isBrackets)
        {
            *output++ = '}';
        }

        *output = '\0';
        return minSize;
    }

    template<size_t SizeT>
    constexpr int Uuid::ToString(char(&output)[SizeT], bool isBrackets, bool isDashes) const
    {
        return ToString(output, SizeT, isBrackets, isDashes);
    }

    /// The only requirements is that StringType can be constructed from char* and it can copied.
    template<class StringType>
    constexpr StringType Uuid::ToString(bool isBrackets, bool isDashes) const
    {
        return StringType{ AZStd::string_view(ToFixedString(isBrackets, isDashes)) };
    }

    /// For inplace version we require resize, data and size members.
    template<class StringType>
    constexpr void Uuid::ToString(StringType& result, bool isBrackets, bool isDashes) const
    {
        FixedString uuidString = ToFixedString(isBrackets, isDashes);
        // Copy the uuid in a fixed_string instance over to the result
        result.assign_range(uuidString);
    }

    constexpr auto Uuid::ToFixedString(bool isBrackets, bool isDashes) const -> FixedString
    {
        auto WriteStringToCharBuffer = [this, isBrackets, isDashes](char* buffer, size_t bufferSize)
        {
            int sizePlusNullTerminator = ToString(buffer, static_cast<int>(bufferSize), isBrackets, isDashes);
            // Must substract out the size of the null-terminating string
            return sizePlusNullTerminator - 1;
        };
        FixedString resultString;
        resultString.resize_and_overwrite(MaxStringBuffer, WriteStringToCharBuffer);

        return resultString;
    }

    template<class R>
    constexpr auto Uuid::CreateData(R&& dataSpan)
        -> AZStd::enable_if_t<AZStd::convertible_to<AZStd::ranges::range_value_t<R>, AZStd::byte>
        || decltype(static_cast<AZStd::byte>(AZStd::declval<AZStd::ranges::range_value_t<R>>()), AZStd::true_type())::value, Uuid>
    {
        if (!dataSpan.empty())
        {
            Sha1 sha;
            for (auto element : dataSpan)
            {
                sha.ProcessByte(static_cast<AZStd::byte>(element));
            }

            AZ::u32 digest[5]{};
            sha.GetDigest(digest);

            Uuid id;
            for (int i = 0; i < 4; ++i)
            {
                id.m_data[i * 4 + 0] = AZStd::byte((digest[i] >> 24) & 0xff);
                id.m_data[i * 4 + 1] = AZStd::byte((digest[i] >> 16) & 0xff);
                id.m_data[i * 4 + 2] = AZStd::byte((digest[i] >> 8) & 0xff);
                id.m_data[i * 4 + 3] = AZStd::byte((digest[i] >> 0) & 0xff);
            }

            // variant VAR_RFC_4122
            id.m_data[8] &= AZStd::byte(0xBF);
            id.m_data[8] |= AZStd::byte(0x80);

            // version VER_NAME_SHA1
            id.m_data[6] &= AZStd::byte(0x5F);
            id.m_data[6] |= AZStd::byte(0x50);

            return id;
        }
        return Uuid{};
    }

    constexpr Uuid Uuid::CreateName(AZStd::string_view name)
    {
        return CreateData(name);
    }

    constexpr Uuid Uuid::CreateData(const AZStd::byte* data, size_t dataSize)
    {
        return CreateData(AZStd::span(data, dataSize));
    }

    constexpr bool Uuid::IsNull() const
    {
        constexpr AZStd::byte zeroBytes[16]{};
        return AZStd::equal(AZStd::begin(m_data), AZStd::end(m_data),
            AZStd::begin(zeroBytes), AZStd::end(zeroBytes));
    }

    constexpr bool Uuid::operator==(const Uuid& rhs) const
    {
        return AZStd::equal(AZStd::begin(m_data), AZStd::end(m_data),
            AZStd::begin(rhs.m_data), AZStd::end(rhs.m_data));
    }
    constexpr bool Uuid::operator!=(const Uuid& rhs) const
    {
        return !operator==(rhs);
    }

    constexpr bool Uuid::operator<(const Uuid& rhs) const
    {
        return AZStd::lexicographical_compare(AZStd::begin(m_data), AZStd::end(m_data),
            AZStd::begin(rhs.m_data), AZStd::end(rhs.m_data));
    }
    constexpr bool Uuid::operator>(const Uuid& rhs) const
    {
        return rhs.operator<(*this);
    }
    constexpr bool Uuid::operator<=(const Uuid& rhs) const
    {
        return !rhs.operator<(*this);
    }
    constexpr bool Uuid::operator>=(const Uuid& rhs) const
    {
        return !operator<(rhs);
    }

    constexpr auto Uuid::GetVariant() const -> Variant
    {
        AZStd::byte val = m_data[8];
        if ((val & AZStd::byte{ 0x80 }) == AZStd::byte{ 0x00 })
        {
            return VAR_NCS;
        }
        else if ((val & AZStd::byte{ 0xC0 }) == AZStd::byte{ 0x80 })
        {
            return VAR_RFC_4122;
        }
        else if ((val & AZStd::byte{ 0xE0 }) == AZStd::byte{ 0xC0 })
        {
            return VAR_MICROSOFT;
        }
        else if ((val & AZStd::byte{ 0xE0 }) == AZStd::byte{ 0xE0 })
        {
            return VAR_RESERVED;
        }
        else
        {
            return VAR_UNKNOWN;
        }
    }

    constexpr auto Uuid::GetVersion() const -> Version
    {
        AZStd::byte val = m_data[6];
        if ((val & AZStd::byte{ 0xF0 }) == AZStd::byte{ 0x10 })
        {
            return VER_TIME;
        }
        else if ((val & AZStd::byte{ 0xF0 }) == AZStd::byte{ 0x20 })
        {
            return VER_DCE;
        }
        else if ((val & AZStd::byte{ 0xF0 }) == AZStd::byte{ 0x30 })
        {
            return VER_NAME_MD5;
        }
        else if ((val & AZStd::byte{ 0xF0 }) == AZStd::byte{ 0x40 })
        {
            return VER_RANDOM;
        }
        else if ((val & AZStd::byte{ 0xF0 }) == AZStd::byte{ 0x50 })
        {
            return VER_NAME_SHA1;
        }
        else
        {
            return VER_UNKNOWN;
        }
    }

    constexpr Uuid operator+(const Uuid& lhs, const Uuid& rhs)
    {
        // Combines both Uuids into 1 single buffer
        AZStd::array<AZStd::byte, sizeof(Uuid::m_data) * 2> mergedData{AZStd::byte{}};
        AZStd::copy(lhs.begin(), lhs.end(), mergedData.begin());
        AZStd::copy(rhs.begin(), rhs.end(), mergedData.begin() + sizeof(Uuid::m_data));
        return Uuid::CreateData(mergedData);
    }

    constexpr Uuid& Uuid::operator+=(const Uuid& rhs)
    {
        *this = *this + rhs;
        return *this;
    }

    // Ranges inter-op functions
    constexpr auto Uuid::begin() -> iterator
    {
        return m_data;
    }
    constexpr auto Uuid::begin() const -> const_iterator
    {
        return m_data;
    }
    constexpr auto Uuid::end() -> iterator
    {
        return m_data + AZ_ARRAY_SIZE(m_data);
    }
    constexpr auto Uuid::end() const -> const_iterator
    {
        return m_data + AZ_ARRAY_SIZE(m_data);
    }
    constexpr size_t Uuid::size() const
    {
        return AZ_ARRAY_SIZE(m_data);
    }

    // Hash function
    constexpr size_t Uuid::GetHash() const
    {
        return GetFirst8BytesAsSizeT();
    }

    // Uuid to size_t chunk functions

    //! Converts the next 8 bytes from the startIndex into a size_t in little endian format
    //! i.e the startIndex position is the low byte and startIndex + 7 is the high byte
    constexpr size_t Uuid::GetSectionAsSizeT(size_t startIndex) const
    {
        return static_cast<size_t>(m_data[startIndex + 7]) << (AZStd::numeric_limits<size_t>::digits - 8)
            | static_cast<size_t>(m_data[startIndex + 6]) << (AZStd::numeric_limits<size_t>::digits - 16)
            | static_cast<size_t>(m_data[startIndex + 5]) << (AZStd::numeric_limits<size_t>::digits - 24)
            | static_cast<size_t>(m_data[startIndex + 4]) << (AZStd::numeric_limits<size_t>::digits - 32)
            | static_cast<size_t>(m_data[startIndex + 3]) << (AZStd::numeric_limits<size_t>::digits - 40)
            | static_cast<size_t>(m_data[startIndex + 2]) << (AZStd::numeric_limits<size_t>::digits - 48)
            | static_cast<size_t>(m_data[startIndex + 1]) << (AZStd::numeric_limits<size_t>::digits - 56)
            | static_cast<size_t>(m_data[startIndex + 0]);
    }

    //! Retrieves the first 8 bytes of the UUID(m_data[7] ... m_data[0]) as a size_t
    //! in little endian format
    constexpr size_t Uuid::GetFirst8BytesAsSizeT() const
    {
        return GetSectionAsSizeT(0);
    }

    //! Retrieves the last 8 bytes of the UUID(m_data[15] ... m_data[8]) as a size_t
    //! in little endian format
    constexpr size_t Uuid::GetLast8BytesAsSizeT() const
    {
        return GetSectionAsSizeT(8);
    }
} // namespace AZ

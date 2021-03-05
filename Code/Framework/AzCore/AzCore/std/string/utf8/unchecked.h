/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Modifications copyright Amazon.com, Inc. or its affiliates.   
// Original code Copyright 2006 Nemanja Trifunovic


#pragma once

#include <AzCore/std/string/utf8/core.h>

namespace Utf8::Unchecked
{
    // Octet iterator class which wraps a source iterator
    template <typename Iterator>
    class octet_iterator
    {
        using iterator_category = AZStd::bidirectional_iterator_tag;
        using value_type = AZ::u32;
        using difference_type = ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
        Iterator it;
    public:
        octet_iterator() = default;
        explicit octet_iterator(const Iterator& sourceIt)
            : it(sourceIt) {}
        // the default "big three" are OK
        Iterator base() const
        {
            return it;
        }
        value_type operator*() const
        {
            return from_utf8_sequence(it);
        }
        bool operator==(const octet_iterator& rhs) const
        {
            return it == rhs.it;
        }
        bool operator!=(const octet_iterator& rhs) const
        {
            return !operator==(rhs);
        }
        octet_iterator& operator++()
        {
            // Skip past the current unicode codepoint
            AZStd::advance(it, Internal::sequence_length(it));
            return *this;
        }
        octet_iterator operator++(int)
        {
            octet_iterator temp = *this;
            // Skip past the current unicode codepoint
            AZStd::advance(it, Internal::sequence_length(it));
            return temp;
        }
        octet_iterator& operator--()
        {
            while (Utf8::Internal::is_trail(*(--it)))
            {
            }
            return *this;
        }
        octet_iterator operator--(int)
        {
            octet_iterator temp = *this;
            while (Utf8::Internal::is_trail(*(--it)))
            {
            }
            return temp;
        }

        static Iterator to_utf8_sequence(AZ::u32 cp, Iterator result)
        {
            if (cp < 0x80)
            {
                // one octet
                *(result++) = static_cast<AZ::u8>(cp);
            }
            else if (cp < 0x800)
            {
                // two octets
                *(result++) = static_cast<AZ::u8>((cp >> 6) | 0xc0);
                *(result++) = static_cast<AZ::u8>((cp & 0x3f) | 0x80);
            }
            else if (cp < 0x10000)
            {
                // three octets
                *(result++) = static_cast<AZ::u8>((cp >> 12) | 0xe0);
                *(result++) = static_cast<AZ::u8>(((cp >> 6) & 0x3f) | 0x80);
                *(result++) = static_cast<AZ::u8>((cp & 0x3f) | 0x80);
            }
            else
            {
                // four octets
                *(result++) = static_cast<AZ::u8>((cp >> 18) | 0xf0);
                *(result++) = static_cast<AZ::u8>(((cp >> 12) & 0x3f) | 0x80);
                *(result++) = static_cast<AZ::u8>(((cp >> 6) & 0x3f) | 0x80);
                *(result++) = static_cast<AZ::u8>((cp & 0x3f) | 0x80);
            }
            return result;
        }

        static AZ::u32 from_utf8_sequence(Iterator it)
        {
            AZ::u32 cp = Utf8::Internal::mask8(*it);
            switch (Utf8::Internal::sequence_length(it))
            {
            case 1:
                break;
            case 2:
                it++;
                cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
                break;
            case 3:
                ++it;
                cp = ((cp << 12) & 0xffff) + ((Utf8::Internal::mask8(*it) << 6) & 0xfff);
                ++it;
                cp += (*it) & 0x3f;
                break;
            case 4:
                ++it;
                cp = ((cp << 18) & 0x1fffff) + ((Utf8::Internal::mask8(*it) << 12) & 0x3ffff);
                ++it;
                cp += (Utf8::Internal::mask8(*it) << 6) & 0xfff;
                ++it;
                cp += (*it) & 0x3f;
                break;
            }
            ++it;
            return cp;
        }
    };

    template <typename u16bit_iterator, typename Utf8Iterator>
    Utf8Iterator utf16to8(u16bit_iterator start, u16bit_iterator end, Utf8Iterator result)
    {
        while (start != end)
        {
            AZ::u32 cp = Utf8::Internal::mask16(*start++);
            // Take care of surrogate pairs first
            if (Utf8::Internal::is_lead_surrogate(cp))
            {
                AZ::u32 trail_surrogate = Utf8::Internal::mask16(*start++);
                cp = (cp << 10) + trail_surrogate + Internal::SURROGATE_OFFSET;
            }
            octet_iterator<Utf8Iterator>::to_utf8_sequence(cp, result);
        }
        return result;
    }

    template <typename u16bit_iterator, typename Utf8Iterator>
    u16bit_iterator utf8to16(Utf8Iterator start, Utf8Iterator end, u16bit_iterator result)
    {
        octet_iterator utf8Start(start);
        octet_iterator utf8Last(end);
        while (utf8Start != utf8Last)
        {
            AZ::u32 cp = *utf8Start++;
            if (cp > 0xffff)
            {
                //make a surrogate pair
                *result++ = static_cast<AZ::u16>((cp >> 10) + Internal::LEAD_OFFSET);
                *result++ = static_cast<AZ::u16>((cp & 0x3ff) + Internal::TRAIL_SURROGATE_MIN);
            }
            else
            {
                *result++ = static_cast<AZ::u16>(cp);
            }
        }
        return result;
    }

    template <typename Utf8Iterator, typename u32bit_iterator>
    Utf8Iterator utf32to8(u32bit_iterator start, u32bit_iterator end, Utf8Iterator result)
    {
        while (start != end)
        {
            octet_iterator<Utf8Iterator>::to_utf8_sequence(*start++, result);
        }

        return result;
    }

    template <typename Utf8Iterator, typename u32bit_iterator>
    u32bit_iterator utf8to32(Utf8Iterator start, Utf8Iterator end, u32bit_iterator result)
    {
        octet_iterator utf8Start(start);
        octet_iterator utf8Last(end);
        while (utf8Start != utf8Last)
        {
            *result++ = *utf8Start++;
        }

        return result;
    }
} // namespace Utf8::Unchecked



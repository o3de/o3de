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

#include <AzCore/std/iterator.h>

namespace Utf8
{
    // Helper code - not intended to be directly called by the library users. May be changed at any time
    namespace Internal
    {
        // Unicode constants
        // Leading (high) surrogates: 0xd800 - 0xdbff
        // Trailing (low) surrogates: 0xdc00 - 0xdfff
        const AZ::u16 LEAD_SURROGATE_MIN = 0xd800u;
        const AZ::u16 LEAD_SURROGATE_MAX = 0xdbffu;
        const AZ::u16 TRAIL_SURROGATE_MIN = 0xdc00u;
        const AZ::u16 TRAIL_SURROGATE_MAX = 0xdfffu;
        const AZ::u16 LEAD_OFFSET = LEAD_SURROGATE_MIN - (0x10000 >> 10);
        const AZ::u32 SURROGATE_OFFSET = 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN;

        // Maximum valid value for a Unicode code point
        const AZ::u32 CODE_POINT_MAX = 0x0010ffffu;

        template<typename octet_type>
        inline AZ::u8 mask8(octet_type oc)
        {
            return static_cast<AZ::u8>(0xff & oc);
        }
        template<typename u16_type>
        inline AZ::u16 mask16(u16_type oc)
        {
            return static_cast<AZ::u16>(0xffff & oc);
        }
        template<typename octet_type>
        inline bool is_trail(octet_type oc)
        {
            return ((Utf8::Internal::mask8(oc) >> 6) == 0x2);
        }

        template <typename u16>
        inline bool is_lead_surrogate(u16 cp)
        {
            return (cp >= LEAD_SURROGATE_MIN && cp <= LEAD_SURROGATE_MAX);
        }

        template <typename u16>
        inline bool is_trail_surrogate(u16 cp)
        {
            return (cp >= TRAIL_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
        }

        template <typename u16>
        inline bool is_surrogate(u16 cp)
        {
            return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
        }

        template <typename u32>
        inline bool is_code_point_valid(u32 cp)
        {
            return (cp <= CODE_POINT_MAX && !Utf8::Internal::is_surrogate(cp));
        }

        template <typename octet_iterator>
        inline typename AZStd::iterator_traits<octet_iterator>::difference_type
            sequence_length(octet_iterator lead_it)
        {
            AZ::u8 lead = Utf8::Internal::mask8(*lead_it);
            if (lead < 0x80)
            {
                return 1;
            }
            else if ((lead >> 5) == 0x6)
            {
                return 2;
            }
            else if ((lead >> 4) == 0xe)
            {
                return 3;
            }
            else if ((lead >> 3) == 0x1e)
            {
                return 4;
            }
            else
            {
                return 0;
            }
        }

        template <typename octet_difference_type>
        inline bool is_overlong_sequence(AZ::u32 cp, octet_difference_type length)
        {
            if (cp < 0x80)
            {
                if (length != 1)
                {
                    return true;
                }
            }
            else if (cp < 0x800)
            {
                if (length != 2)
                {
                    return true;
                }
            }
            else if (cp < 0x10000)
            {
                if (length != 3)
                {
                    return true;
                }
            }

            return false;
        }

        enum utf_error { UTF8_OK, NOT_ENOUGH_ROOM, INVALID_LEAD, INCOMPLETE_SEQUENCE, OVERLONG_SEQUENCE, INVALID_CODE_POINT };

        /// Helper for get_sequence_x
        template <typename octet_iterator>
        utf_error increase_safely(octet_iterator& it, octet_iterator end)
        {
            if (++it == end)
            {
                return NOT_ENOUGH_ROOM;
            }

            if (!Utf8::Internal::is_trail(*it))
            {
                return INCOMPLETE_SEQUENCE;
            }

            return UTF8_OK;
        }

#define UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(IT, END) {utf_error ret = increase_safely(IT, END); if (ret != UTF8_OK) return ret;}    

        /// get_sequence_x functions decode utf-8 sequences of the length x
        template <typename octet_iterator>
        utf_error get_sequence_1(octet_iterator& it, octet_iterator end, AZ::u32& code_point)
        {
            if (it == end)
            {
                return NOT_ENOUGH_ROOM;
            }

            code_point = Utf8::Internal::mask8(*it);

            return UTF8_OK;
        }

        template <typename octet_iterator>
        utf_error get_sequence_2(octet_iterator& it, octet_iterator end, AZ::u32& code_point)
        {
            if (it == end)
            {
                return NOT_ENOUGH_ROOM;
            }

            code_point = Utf8::Internal::mask8(*it);

            UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end);

            code_point = ((code_point << 6) & 0x7ff) + ((*it) & 0x3f);

            return UTF8_OK;
        }

        template <typename octet_iterator>
        utf_error get_sequence_3(octet_iterator& it, octet_iterator end, AZ::u32& code_point)
        {
            if (it == end)
            {
                return NOT_ENOUGH_ROOM;
            }

            code_point = Utf8::Internal::mask8(*it);

            UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end);

            code_point = ((code_point << 12) & 0xffff) + ((Utf8::Internal::mask8(*it) << 6) & 0xfff);

            UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end);

            code_point += (*it) & 0x3f;

            return UTF8_OK;
        }

        template <typename octet_iterator>
        utf_error get_sequence_4(octet_iterator& it, octet_iterator end, AZ::u32& code_point)
        {
            if (it == end)
            {
                return NOT_ENOUGH_ROOM;
            }

            code_point = Utf8::Internal::mask8(*it);

            UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end);

            code_point = ((code_point << 18) & 0x1fffff) + ((Utf8::Internal::mask8(*it) << 12) & 0x3ffff);

            UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end);

            code_point += (Utf8::Internal::mask8(*it) << 6) & 0xfff;

            UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end);

            code_point += (*it) & 0x3f;

            return UTF8_OK;
        }

#undef UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR

        template <typename octet_iterator>
        utf_error validate_next(octet_iterator& it, octet_iterator end, AZ::u32& code_point)
        {
            // Save the original value of it so we can go back in case of failure
            // Of course, it does not make much sense with i.e. stream iterators
            octet_iterator original_it = it;

            AZ::u32 cp = 0;
            // Determine the sequence length based on the lead octet
            typedef typename AZStd::iterator_traits<octet_iterator>::difference_type octet_difference_type;
            const octet_difference_type length = Utf8::Internal::sequence_length(it);

            // Get trail octets and calculate the code point
            utf_error err = UTF8_OK;
            switch (length)
            {
            case 0:
                return INVALID_LEAD;
            case 1:
                err = Utf8::Internal::get_sequence_1(it, end, cp);
                break;
            case 2:
                err = Utf8::Internal::get_sequence_2(it, end, cp);
                break;
            case 3:
                err = Utf8::Internal::get_sequence_3(it, end, cp);
                break;
            case 4:
                err = Utf8::Internal::get_sequence_4(it, end, cp);
                break;
            }

            if (err == UTF8_OK)
            {
                // Decoding succeeded. Now, security checks...
                if (Utf8::Internal::is_code_point_valid(cp))
                {
                    if (!Utf8::Internal::is_overlong_sequence(cp, length))
                    {
                        // Passed! Return here.
                        code_point = cp;
                        ++it;
                        return UTF8_OK;
                    }
                    else
                    {
                        err = OVERLONG_SEQUENCE;
                    }
                }
                else
                {
                    err = INVALID_CODE_POINT;
                }
            }

            // Failure branch - restore the original value of the iterator
            it = original_it;
            return err;
        }

        template <typename octet_iterator>
        inline utf_error validate_next(octet_iterator& it, octet_iterator end)
        {
            AZ::u32 ignored;
            return Utf8::Internal::validate_next(it, end, ignored);
        }

    } // namespace Internal

    /// The library API - functions intended to be called by the users

    // Byte order mark
    const AZ::u8 bom[] = { 0xef, 0xbb, 0xbf };

    template <typename octet_iterator>
    octet_iterator find_invalid(octet_iterator start, octet_iterator end)
    {
        octet_iterator result = start;
        while (result != end)
        {
            Utf8::Internal::utf_error err_code = Utf8::Internal::validate_next(result, end);
            if (err_code != Internal::UTF8_OK)
            {
                return result;
            }
        }
        return result;
    }

    template <typename octet_iterator>
    inline bool is_valid(octet_iterator start, octet_iterator end)
    {
        return (Utf8::find_invalid(start, end) == end);
    }

    template <typename octet_iterator>
    inline bool starts_with_bom(octet_iterator it, octet_iterator end)
    {
        return (
            ((it != end) && (Utf8::Internal::mask8(*it++)) == bom[0]) &&
            ((it != end) && (Utf8::Internal::mask8(*it++)) == bom[1]) &&
            ((it != end) && (Utf8::Internal::mask8(*it)) == bom[2])
            );
    }
} // namespace utf8

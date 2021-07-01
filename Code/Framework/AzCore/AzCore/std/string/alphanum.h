/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_STRING_ALPHANUM_H
#define AZSTD_STRING_ALPHANUM_H

#include <AzCore/std/string/string.h>
#include <AzCore/std/functional_basic.h>

namespace AZStd
{
    namespace Internal
    {
        /**
        * compare l and r with strcmp() semantics, but using
        * the "Alphanum Algorithm". This function is designed to read
        * through the l and r strings only one time, for
        * maximum performance. It does not allocate memory for
        * substrings. It can either use the C-library functions isdigit()
        * and atoi() to honour your locale settings, when recognizing
        * digit characters when you "#define ALPHANUM_LOCALE=1" or use
        * it's own digit character handling which only works with ASCII
        * digit characters, but provides better performance.
        *
        * @param l NULL-terminated C-style string
        * @param r NULL-terminated C-style string
        * @return negative if l<r, 0 if l equals r, positive if l>r
        *
        */
        int alphanum_impl(const char* l, const char* r);
    }

    /**
    * Compare l and r with the same semantics as strcmp(), but with
    * the "Alphanum Algorithm" which produces more human-friendly
    * results.
    * @return negative if l<r, 0 if l==r, positive if l>r.
    */
    template<class Element, class Traits, class Allocator1, class Allocator2>
    AZ_FORCE_INLINE int alphanum_comp(const basic_string< Element, Traits, Allocator1>& l, const basic_string< Element, Traits, Allocator2>& r)
    {
        return Internal::alphanum_impl(l.c_str(), r.c_str());
    }

    AZ_FORCE_INLINE int alphanum_comp(const char* l, const char* r) { return Internal::alphanum_impl(l, r); }
    template<class Element, class Traits, class Allocator>
    AZ_FORCE_INLINE int alphanum_comp(const basic_string< Element, Traits, Allocator>& l, const char* r)  { return Internal::alphanum_impl(l.c_str(), r); }
    template<class Element, class Traits, class Allocator>
    AZ_FORCE_INLINE int alphanum_comp(const char* l, const basic_string< Element, Traits, Allocator>& r)   { return Internal::alphanum_impl(l, r.c_str()); }

    /// Functor class to compare two objects with the "Alphanum Algorithm".
    template<class T>
    struct alphanum_less
    {
        bool operator()(const T& left, const T& right) const
        {
            return alphanum_comp(left, right) < 0;
        }
    };
} // namespace AZStd

#endif // AZSTD_STRING_ALPHANUM_H


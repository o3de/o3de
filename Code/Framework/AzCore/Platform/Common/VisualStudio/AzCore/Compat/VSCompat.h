/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


// This file is force-included into all VS compiles, and can be used to place 
// macros and templates that help compatibility issues affecting new compiler releases,
// for example, deprecation of symbols during a release cycle that would otherwise
// impact thousands of files.

// Adding or removing from this file will have global impact.
// Additions to this file should have a known end of life.

// ----------------------------------------------------
// Section 1:  remove when we are on Qt6 exclusively
// ----------------------------------------------------
// QT5 unconditionally uses the (removed) stdext::make_checked_array_iterator
#if defined(__cplusplus) && _MSC_VER >= 1938  // stdext is deprecated since VS 2022 17.8
namespace stdext
{
    template<class _Ptr>
    [[nodiscard]]
    constexpr _Ptr make_checked_array_iterator(_Ptr x, [[maybe_unused]] size_t size, [[maybe_unused]] size_t z = 0) noexcept
    {
        return x;
    }
} // namespace stdext
#endif

// ----------------------------------------------------
// Section 1: end
// ----------------------------------------------------

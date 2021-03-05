// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// Portions of the code in this file is a derived work based on the
// FunctionRef class in LLVM:
//
// University of Illinois/NCSA Open Source License
//
// Copyright (c) 2003-2018 University of Illinois at Urbana-Champaign.
// All rights reserved.
//
// Developed by:
//   LLVM Team, University of Illinois at Urbana-Champaign, http://llvm.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal with
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimers.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimers in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the names of the LLVM Team, University of Illinois at
//       Urbana-Champaign, nor the names of its contributors may be used to
//       endorse or promote products derived from this Software without specific
//       prior written permission.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE.


#pragma once

#include <functional>
#include <type_traits>

#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/platform.h>


OIIO_NAMESPACE_BEGIN


/// function_view<R(T...)> is a lightweight non-owning generic callable
/// object view, similar to a std::function<R(T...)>, but with much less
/// overhead.
///
/// A function_view invocation should have the same cost as a function
/// pointer (which it basically is underneath). Similar in spirit to a
/// string_view or span, the function-like object that the function_view
/// refers to MUST have a lifetime that outlasts any use of the
/// function_view.
///
/// In contrast, a full std::function<> is an owning container for a
/// callable object. It's more robust, especially with restpect to object
/// lifetimes, but the call overhead is quite high. So use a function_view
/// when you can.
///
/// This implementation comes from LLVM:
/// https://github.com/llvm-mirror/llvm/blob/master/include/llvm/ADT/STLExtras.h

template<typename Fn> class function_view;

template<typename Ret, typename... Params> class function_view<Ret(Params...)> {
    Ret (*callback)(intptr_t callable, Params... params) = nullptr;
    intptr_t callable;

    template<typename Callable>
    static Ret callback_fn(intptr_t callable, Params... params)
    {
        return (*reinterpret_cast<Callable*>(callable))(
            std::forward<Params>(params)...);
    }

public:
    function_view() = default;
    function_view(std::nullptr_t) {}

    template<typename Callable>
    function_view(
        Callable&& callable,
        typename std::enable_if<
            !std::is_same<typename std::remove_reference<Callable>::type,
                          function_view>::value>::type* = nullptr)
        : callback(callback_fn<typename std::remove_reference<Callable>::type>)
        , callable(reinterpret_cast<intptr_t>(&callable))
    {
    }

    Ret operator()(Params... params) const
    {
        return callback(callable, std::forward<Params>(params)...);
    }

    operator bool() const { return callback; }
};


OIIO_NAMESPACE_END

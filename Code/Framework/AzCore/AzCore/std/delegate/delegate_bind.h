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
#ifndef AZSTD_DELEGATE_BIND_H
#define AZSTD_DELEGATE_BIND_H

// Based on FastDelegate v. 1.5 from Don Clugston. (http://www.codeproject.com/cpp/FastDelegate.asp)
//
// Warning: The arguments to 'bind' are ignored! No actual binding is performed.
// The behaviour is equivalent to boost::bind only when the basic placeholder
// arguments _1, _2, _3, etc are used in order.
//
// Add another helper, so delegate can be a drop in replacement
// for boost::bind (in a fair number of cases).
// Note the ellipses, because boost::bind() takes place holders
// but delegate does not care about them.  Getting the place holder
// mechanism to work, and play well with boost is a bit tricky, so
// we do the "easy" thing...
// Assume we have the following code...
//      using AZStd (or tr1 or boost)::bind;
//      bind(&Foo:func, &foo, _1, _2);
// we should be able to replace the "using" with...
//      using delegate_util::bind;
// and everything should work fine...
////////////////////////////////////////////////////////////////////////////////

namespace AZStd
{
    namespace delegate_util
    {
        //N=0
        template <class X, class Y, class R>
        delegate< R () >
        bind(
            R (X::* func)(),
            Y* y,
            ...)
        {
            return delegate< R () >(y, func);
        }

        template <class X, class Y, class R>
        delegate< R () >
        bind(
            R (X::* func)() const,
            Y* y,
            ...)
        {
            return delegate< R () >(y, func);
        }

        //N=1
        template <class X, class Y, class R, class Param1>
        delegate< R (Param1 p1) >
        bind(
            R (X::* func)(Param1 p1),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1) >(y, func);
        }

        template <class X, class Y, class R, class Param1>
        delegate< R (Param1 p1) >
        bind(
            R (X::* func)(Param1 p1) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1) >(y, func);
        }

        //N=2
        template <class X, class Y, class R, class Param1, class Param2>
        delegate< R (Param1 p1, Param2 p2) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2) >(y, func);
        }

        template <class X, class Y, class R, class Param1, class Param2>
        delegate< R (Param1 p1, Param2 p2) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2) >(y, func);
        }

        //N=3
        template <class X, class Y, class R, class Param1, class Param2, class Param3>
        delegate< R (Param1 p1, Param2 p2, Param3 p3) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3) >(y, func);
        }

        template <class X, class Y, class R, class Param1, class Param2, class Param3>
        delegate< R (Param1 p1, Param2 p2, Param3 p3) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3) >(y, func);
        }

        //N=4
        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4) >(y, func);
        }

        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4) >(y, func);
        }

        //N=5
        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5) >(y, func);
        }

        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5) >(y, func);
        }

        //N=6
        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5, class Param6>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6) >(y, func);
        }

        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5, class Param6>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6) >(y, func);
        }

        //N=7
        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5, class Param6, class Param7>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7) >(y, func);
        }

        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5, class Param6, class Param7>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7) >(y, func);
        }

        //N=8
        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5, class Param6, class Param7, class Param8>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8),
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8) >(y, func);
        }

        template <class X, class Y, class R, class Param1, class Param2, class Param3, class Param4, class Param5, class Param6, class Param7, class Param8>
        delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8) >
        bind(
            R (X::* func)(Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8) const,
            Y* y,
            ...)
        {
            return delegate< R (Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8) >(y, func);
        }
    }
}

#endif //AZSTD_DELEGATE_BIND_H
#pragma once
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SCOPEDCLEANUP_H
#define SCOPEDCLEANUP_H

#include <QtCore/qglobal.h>

/**
 * RAII-style class that executes a lambda when it goes out of scope.
 * Ideal for cleanup in functions that have multiple return points.
 *
 * Usage:
 *     #include <AzQtComponents/Utilities/ScopedCleanup.h>
 *     (...)
 *     auto cleanup = scopedCleanup([]{ <my cleanup that should be executed at end of scope>});
 */

template <typename F> class ScopedCleanup;
template <typename F> ScopedCleanup<F> scopedCleanup(F f);

template <typename F>
class ScopedCleanup
{
public:
    ScopedCleanup(ScopedCleanup&& other) Q_DECL_NOEXCEPT
        : m_func(std::move(other.m_func))
        , m_invoke(other.m_invoke)
    {
        other.dismiss();
    }

    ~ScopedCleanup()
    {
        if (m_invoke)
        {
            m_func();
        }
    }

    void dismiss() Q_DECL_NOEXCEPT
    {
        m_invoke = false;
    }

private:
    explicit ScopedCleanup(F f) Q_DECL_NOEXCEPT
        : m_func(std::move(f))
    {
    }

    Q_DISABLE_COPY(ScopedCleanup)

    F m_func;
    bool m_invoke = true;
    friend ScopedCleanup scopedCleanup<F>(F);
};


template <typename F>
ScopedCleanup<F> scopedCleanup(F f)
{
    return ScopedCleanup<F>(std::move(f));
}

#endif

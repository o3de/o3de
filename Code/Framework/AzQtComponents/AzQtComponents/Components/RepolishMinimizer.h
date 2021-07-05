/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QStyle>

/**
 * RAII-style class that will block unneeded polish requests when doing reparenting.
 * When QWidget::setParent() is called that triggers all children to be repolished, which is expensive
 * as all stylesheet rules have to be recalculated.
 *
 * Repolishing is usually only needed if the stylesheet changed. You can use this class to save CPU cycles
 * when you know you're going to trigger setParent() calls that you're sure wouldn't affect any styling.
 */
namespace AzQtComponents
{
    class RepolishMinimizer
    {
    public:
        RepolishMinimizer()
        {
#if !defined(AZ_PLATFORM_LINUX)
            // Enable optimizations
            QStyle::enableMinimizePolishOptimizations(true);
#endif // !defined(AZ_PLATFORM_LINUX)
        }

        ~RepolishMinimizer()
        {
#if !defined(AZ_PLATFORM_LINUX)
            // Disable optimizations. Back to normal.
            QStyle::enableMinimizePolishOptimizations(false);
#endif // !defined(AZ_PLATFORM_LINUX)
        }

    private:
        Q_DISABLE_COPY(RepolishMinimizer)
    };

} // namespace AzQtComponents

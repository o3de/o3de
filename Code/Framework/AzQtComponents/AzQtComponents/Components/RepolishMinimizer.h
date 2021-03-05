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

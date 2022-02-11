/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/RTTI/ReflectContext.h>
#include <AzQtComponents/Components/ComponentReflectionSpecialization.h>
#include <QFrame>
#endif

class QPainter;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ButtonDivider
        : public QFrame
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ButtonDivider, AZ::SystemAllocator, 0);
        AZ_RTTI(ButtonDivider, "{C59F781E-50D2-474E-B87C-D690E661C172}");

        explicit ButtonDivider(QWidget* parent = nullptr);

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace AzQtComponents



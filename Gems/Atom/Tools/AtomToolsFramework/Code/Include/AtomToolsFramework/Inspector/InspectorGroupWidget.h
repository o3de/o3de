/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <QWidget>
#endif

namespace AtomToolsFramework
{
    class InspectorGroupWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(InspectorGroupWidget, AZ::SystemAllocator);

        InspectorGroupWidget(QWidget* parent = nullptr);

        //! Apply non-destructive UI changes like repainting or updating fields
        virtual void Refresh();

        //! Apply destructive UI changes like adding or removing child widgets
        virtual void Rebuild();
    };
} // namespace AtomToolsFramework

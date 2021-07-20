/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZAUTOSIZINGSCROLLAREA_HXX
#define AZAUTOSIZINGSCROLLAREA_HXX

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

#include <QtWidgets/QScrollArea>
#endif

namespace AzToolsFramework
{
    // This fixes a bug in QScrollArea which makes it so that you can dynamically add and remove elements from inside it, and the scroll
    // area will take up as much room as it needs to, to prevent the need for scroll bars.  Scroll bars will still appear if there is not enough
    // room, but the view will scale up to eat all available room before that happens.

    // QScrollArea was supposed to do this, but it appears to cache the size of its embedded widget on startup, and never clears that cache.
    class AZAutoSizingScrollArea
        : public QScrollArea
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AZAutoSizingScrollArea, AZ::SystemAllocator, 0);

        explicit AZAutoSizingScrollArea(QWidget* parent = 0);

        QSize sizeHint() const;
    };
}

#endif

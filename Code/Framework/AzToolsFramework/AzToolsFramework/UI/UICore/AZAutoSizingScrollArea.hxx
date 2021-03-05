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
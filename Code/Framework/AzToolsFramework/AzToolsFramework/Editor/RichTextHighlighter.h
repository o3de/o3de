/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>
#include <QString>
#include <QStyleOptionViewItem>
#include <QTextDocument>

AZ_PUSH_DISABLE_WARNING(4251 4800,"-Wunknown-warning-option") // 4251: class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used
                                                              // by clients of class 'QBrush' 4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
#include <QPainter>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    //! @class RichTextHighlighter
    //! @brief Highlights a given string given a matching substring.
    class RichTextHighlighter
    {
    public:
        AZ_CLASS_ALLOCATOR(RichTextHighlighter, AZ::SystemAllocator);
        RichTextHighlighter() = delete;

        static QString HighlightText(const QString& displayString, const QString& matchingSubstring);
        static void PaintHighlightedRichText(const QString& highlightedString,QPainter* painter, QStyleOptionViewItem option,
            QRect availableRect, QPoint offset = QPoint());

    };
} // namespace AzToolsFramework

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DHUIFRAMEWORK_PLAINTEXTEDIT_HXX
#define DHUIFRAMEWORK_PLAINTEXTEDIT_HXX

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4244 4800, "-Wunknown-warning-option") // 4251: 'QRawFont::d': class 'QExplicitlySharedDataPointer<QRawFontPrivate>' needs to have dll-interface to be used by clients of class 'QRawFont'
                                                                    // 4244: conversion from 'int' to 'float', possible loss of data
                                                                    // 4800: 'QTextEngine *const ': forcing value to bool 'true' or 'false' (performance warning)
#include <QtWidgets/qplaintextedit.h>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    //Just provides access to some protected functionality, and some convenience functions.
    class PlainTextEdit
        : public QPlainTextEdit
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PlainTextEdit, AZ::SystemAllocator, 0);
        explicit PlainTextEdit(QWidget* parent = 0)
            : QPlainTextEdit(parent) {}

        QRectF GetBlockBoundingGeometry(const QTextBlock& block) const;
        void ForEachVisibleBlock(AZStd::function<void(QTextBlock& block, const QRectF&)> operation) const;

    private:
        void scrollContentsBy(int, int) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    signals:
        void Scrolled();
        //accept the event to avoid default double click behavior
        void BlockDoubleClicked(QMouseEvent* event, const QTextBlock& block);
    };
}

#endif

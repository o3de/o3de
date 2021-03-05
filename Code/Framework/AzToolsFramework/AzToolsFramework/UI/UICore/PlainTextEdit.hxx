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
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#ifndef AZTOOLSFRAMEWORK_UI_UICORE_CLICKABLELABEL_H
#define AZTOOLSFRAMEWORK_UI_UICORE_CLICKABLELABEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
#include <QLabel>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QMouseEvent>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    class ClickableLabel
        : public QLabel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ClickableLabel, AZ::SystemAllocator);
        explicit ClickableLabel(QWidget* parent = nullptr);
        ~ClickableLabel();

    signals:
        void clicked();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

    private:
        bool m_pressed;
    };
}

#endif

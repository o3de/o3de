/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
#include <QTimer>
#include <QIcon>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'qint8', possible loss of data
                                                                // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QHBoxLayout>
AZ_POP_DISABLE_WARNING

namespace Ui
{
    class AssetEditorHeader
        : public QFrame
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(AssetEditorHeader, AZ::SystemAllocator);
       
        explicit AssetEditorHeader(QWidget* parent = nullptr);

        void setName(const QString& name);
        void setIcon(const QIcon& icon);
    private:
        QFrame* m_backgroundFrame;
        QHBoxLayout* m_backgroundLayout;
        QVBoxLayout* m_mainLayout;
        QLabel* m_iconLabel;

        QIcon m_icon;

        AzQtComponents::ElidingLabel* m_assetName;
    };
} // namespace Ui

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZ_COMBOBOX_HXX
#define AZ_COMBOBOX_HXX

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QComboBox>
AZ_POP_DISABLE_WARNING

#pragma once

namespace AzToolsFramework
{
    class DHQComboBox
        : public QComboBox
    {
    public:
        AZ_CLASS_ALLOCATOR(DHQComboBox, AZ::SystemAllocator);

        explicit DHQComboBox(QWidget* parent = 0);

        void showPopup() override;

        void SetHeaderOverride(const QString& overrideString);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void wheelEvent(QWheelEvent* e) override;
        bool event(QEvent* event) override;

        QString m_headerOverride;
    };
}

#endif

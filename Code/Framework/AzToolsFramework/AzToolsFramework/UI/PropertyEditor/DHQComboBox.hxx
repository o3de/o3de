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
        AZ_CLASS_ALLOCATOR(DHQComboBox, AZ::SystemAllocator, 0);

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

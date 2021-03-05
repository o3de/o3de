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

#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QLabel>
#include <QWidget>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    //! Widget for adjusting Field of View in viewport
    class FovSliderWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        FovSliderWidget(QWidget* parent = 0);
        ~FovSliderWidget() = default;

    private:
        QLabel* m_label;

    private Q_SLOTS:
        void SliderValueChanged(int value) const;
    };
} // namespace MaterialEditor

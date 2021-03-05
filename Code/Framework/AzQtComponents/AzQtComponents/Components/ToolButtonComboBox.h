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

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/ToolButtonWithWidget.h>
#endif

class QComboBox;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ToolButtonComboBox
        : public ToolButtonWithWidget
    {
        Q_OBJECT

    public:
        explicit ToolButtonComboBox(QWidget* parent = nullptr);
        QComboBox* comboBox() const;

    private:
        QComboBox* const m_combo;
    };
} // namespace AzQtComponents


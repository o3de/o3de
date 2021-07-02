/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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


/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once
#include "Tools/WhiteBoxModelingWindow.h"
#include <AzCore/Component/ComponentBus.h>

class QDoubleSpinBox;
class QCheckBox;
class QLabel;
class QPushButton;

namespace WhiteBox
{
    class WhiteBoxExtrudeInsetWindow : public WhiteBoxModelingWindow
    {
    public:
        WhiteBoxExtrudeInsetWindow(const AZ::EntityComponentIdPair& pair, bool inset, QWidget* parent);
        void accept() override;
        void reject() override;

    private:
        bool HasCurrentLayer() const;
        void RefreshSelection();
        AZ::EntityComponentIdPair m_pair;
        AZ::u64 m_layerId = 0;
        bool m_inset = false;
        QDoubleSpinBox* m_amount = nullptr;
        QCheckBox* m_latch = nullptr;
        QLabel* m_selection = nullptr;
        QLabel* m_status = nullptr;
        QPushButton* m_apply = nullptr;
    };
}

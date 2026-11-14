/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include "Tools/WhiteBoxModelingWindow.h"
#include <AzCore/Component/ComponentBus.h>

class QComboBox;
class QLabel;
class QPushButton;

namespace WhiteBox
{
    //! Options for the existing undoable Weld operation; opening it does not change the mesh.
    class WhiteBoxWeldWindow : public WhiteBoxModelingWindow
    {
    public:
        WhiteBoxWeldWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent);
        bool HasCurrentLayer() const;
        void accept() override;
        void reject() override;

    private:
        void RefreshSelection();

        AZ::EntityComponentIdPair m_pair;
        AZ::u64 m_layerId = 0;
        QComboBox* m_target = nullptr;
        QLabel* m_selection = nullptr;
        QLabel* m_status = nullptr;
        QPushButton* m_weld = nullptr;
    };
}

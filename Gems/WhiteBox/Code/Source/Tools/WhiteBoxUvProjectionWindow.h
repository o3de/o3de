/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include "Tools/WhiteBoxModelingWindow.h"
#include <WhiteBox/WhiteBoxToolApi.h>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace WhiteBox
{
    //! Non-modal texture placement for the selected polygons: projection mode, tiling, offset and rotation.
    class WhiteBoxUvProjectionWindow : public WhiteBoxModelingWindow
    {
    public:
        WhiteBoxUvProjectionWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent);
        bool HasCurrentLayer() const;
        void reject() override;

    private:
        //! Show the first selected polygon's settings, unless a value is being typed.
        void RefreshSelection(bool force = false);
        void LoadValues(const Api::UvProjection& projection);
        Api::UvProjection ControlValues() const;
        void Apply();
        void ShowResult(bool success, const AZStd::string& message);

        AZ::EntityComponentIdPair m_pair;
        AZ::u64 m_layerId = 0;
        QComboBox* m_mode = nullptr;
        QDoubleSpinBox* m_tilingU = nullptr;
        QDoubleSpinBox* m_tilingV = nullptr;
        QDoubleSpinBox* m_offsetU = nullptr;
        QDoubleSpinBox* m_offsetV = nullptr;
        QDoubleSpinBox* m_rotation = nullptr;
        QPushButton* m_fit = nullptr;
        QPushButton* m_reset = nullptr;
        QLabel* m_selection = nullptr;
        QLabel* m_status = nullptr;
    };
} // namespace WhiteBox

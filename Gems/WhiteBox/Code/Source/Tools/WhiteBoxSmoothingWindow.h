/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/array.h>
#include "Tools/WhiteBoxModelingWindow.h"

class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace WhiteBox
{
    //! Non-modal smoothing group editor for the selected polygons: 32 group toggles, Clear and Auto Smooth.
    class WhiteBoxSmoothingWindow : public WhiteBoxModelingWindow
    {
    public:
        WhiteBoxSmoothingWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent);
        bool HasCurrentLayer() const;
        void reject() override;

    private:
        static constexpr int GroupCount = 32;

        //! Light each group every selected polygon has; outline the ones only some have.
        void RefreshSelection();
        void ToggleGroup(int group);
        void ShowResult(bool success, const AZStd::string& message);

        AZ::EntityComponentIdPair m_pair;
        AZ::u64 m_layerId = 0;
        AZStd::array<QPushButton*, GroupCount> m_groups{};
        AZ::u32 m_shownAll = ~0u; //!< What the toggles last showed, so the timer only restyles on change.
        AZ::u32 m_shownAny = ~0u;
        QPushButton* m_clear = nullptr;
        QPushButton* m_autoSmooth = nullptr;
        QDoubleSpinBox* m_angle = nullptr;
        QLabel* m_selection = nullptr;
        QLabel* m_status = nullptr;
    };
} // namespace WhiteBox

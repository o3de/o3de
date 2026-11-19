/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include "Tools/WhiteBoxModelingWindow.h"

class QDoubleSpinBox;
class QSpinBox;
class QSlider;
class QLabel;

namespace WhiteBox
{
    class EditorWhiteBoxComponent;

    //! Non-modal controls for the active layer's saved bevel modifier.
    class WhiteBoxBevelWindow : public WhiteBoxModelingWindow
    {
    public:
        WhiteBoxBevelWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent);
        bool HasCurrentBevel() const;
        void accept() override;
        void reject() override;

    private:
        EditorWhiteBoxComponent* CurrentComponent() const;
        void RefreshValues(bool force = false);
        void UpdateBevel();
        void FinishEdit();

        AZ::EntityComponentIdPair m_pair;
        AZ::u64 m_layerId = 0;
        QDoubleSpinBox* m_width = nullptr;
        QSpinBox* m_segments = nullptr;
        QDoubleSpinBox* m_profile = nullptr;
        QSlider* m_widthSlider = nullptr;
        QSlider* m_segmentsSlider = nullptr;
        QSlider* m_profileSlider = nullptr;
        QLabel* m_status = nullptr;
        double m_widthRange = 1.0;
        bool m_dirty = false;
    };
}

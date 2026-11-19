/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/vector.h>
#include "SubComponentModes/WhiteBoxPaintSettings.h"
#include "Tools/WhiteBoxModelingWindow.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QWidget;

namespace AzToolsFramework
{
    class PropertyAssetCtrl;
}

namespace WhiteBox
{
    class EditorWhiteBoxComponent;

    //! The paint brush payload (material or colour); the operation itself is picked in the viewport cluster.
    class WhiteBoxPaintWindow : public WhiteBoxModelingWindow
    {
    public:
        WhiteBoxPaintWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent);
        //! Re-read the component after the cluster (or the pane) changes the operation.
        void RefreshValues();

    private:
        EditorWhiteBoxComponent* CurrentComponent() const;
        void ChooseColor();
        //! Paint the swatch button with @p color so the window shows what it will apply.
        void ApplySwatch(AZ::u32 color);
        void ApplyVisibility(FacePaintOperation operation);
        //! Refill the combo from the card's list when it changed, and select the brush's material.
        void RefreshMaterialList();
        void ChooseMaterial(int row);
        void AddMaterial(const AZ::Data::AssetId& material);

        AZ::EntityComponentIdPair m_pair;
        QLabel* m_title = nullptr;
        QLabel* m_hint = nullptr;
        QLabel* m_materialLabel = nullptr;
        QComboBox* m_material = nullptr;               //!< The card's material list; the brush paints the chosen one.
        AZStd::vector<AZ::Data::AssetId> m_materialIds; //!< What each combo row paints with.
        QLabel* m_addMaterialLabel = nullptr;
        AzToolsFramework::PropertyAssetCtrl* m_addMaterial = nullptr; //!< Picking here adds to the list.
        QLabel* m_colorLabel = nullptr;
        QPushButton* m_color = nullptr;
        QCheckBox* m_wholePolygon = nullptr;
        QLabel* m_radiusLabel = nullptr;
        QDoubleSpinBox* m_radius = nullptr;
        QLabel* m_strengthLabel = nullptr;
        QDoubleSpinBox* m_strength = nullptr;
        QLabel* m_hardnessLabel = nullptr;
        QDoubleSpinBox* m_hardness = nullptr;
        bool m_updating = false;
    };
} // namespace WhiteBox

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QBrush>
#include <QColor>
#include <QPalette>

// ----------------------------------------------------------------------------------
// The palettes the .ui forms of the two asset details panels gave the "AssetDetailsSeparator" line. The forms no longer hold them: each
// panel sets its palette in code right after setupUi, in the legacy and in the flattened GUI, from the colours named here. The header
// is shared because the panels are classes of the application target and the tests (AssetProcessor.Tests) cannot build them: the
// tests build the line the way the panels do and compare its palette with the values the forms held (DetailsPanelPaletteTests.cpp).
// ----------------------------------------------------------------------------------

namespace AssetProcessor
{
    // ------------------------------------------------------------------------------
    // The palette of the separator line: the three colour groups of the forms (Active, Inactive, Disabled), each with the line colour
    // for WindowText and one fill that Button, Base and Window all took. Every brush of the forms is solid.
    // ------------------------------------------------------------------------------

    inline QPalette MakeSeparatorPalette(
        const QColor& lineActive, const QColor& lineInactive, const QColor& lineDisabled, const QColor& fill)
    {
        const QBrush lineActiveBrush(lineActive, Qt::SolidPattern);
        const QBrush lineInactiveBrush(lineInactive, Qt::SolidPattern);
        const QBrush lineDisabledBrush(lineDisabled, Qt::SolidPattern);
        const QBrush fillBrush(fill, Qt::SolidPattern);

        QPalette palette;
        palette.setBrush(QPalette::Active, QPalette::WindowText, lineActiveBrush);
        palette.setBrush(QPalette::Active, QPalette::Button, fillBrush);
        palette.setBrush(QPalette::Active, QPalette::Base, fillBrush);
        palette.setBrush(QPalette::Active, QPalette::Window, fillBrush);
        palette.setBrush(QPalette::Inactive, QPalette::WindowText, lineInactiveBrush);
        palette.setBrush(QPalette::Inactive, QPalette::Button, fillBrush);
        palette.setBrush(QPalette::Inactive, QPalette::Base, fillBrush);
        palette.setBrush(QPalette::Inactive, QPalette::Window, fillBrush);
        palette.setBrush(QPalette::Disabled, QPalette::WindowText, lineDisabledBrush);
        palette.setBrush(QPalette::Disabled, QPalette::Button, fillBrush);
        palette.setBrush(QPalette::Disabled, QPalette::Base, fillBrush);
        palette.setBrush(QPalette::Disabled, QPalette::Window, fillBrush);
        return palette;
    }

    // ------------------------------------------------------------------------------
    // ProductAssetDetailsPanel: ProductAssetDetailsPanel.ui:58-175 (the line's <palette>)
    // ------------------------------------------------------------------------------

    namespace ProductAssetDetailsPanelFlat
    {
        // ProductAssetDetailsPanel.ui:63-67 (Active, WindowText). TOKENIZE: ProductPanelSeparatorLineActive
        inline constexpr QColor SeparatorLineActive(255, 255, 255, 255);

        // ProductAssetDetailsPanel.ui:101-105 (Inactive, WindowText). TOKENIZE: ProductPanelSeparatorLineInactive
        inline constexpr QColor SeparatorLineInactive(255, 255, 255, 255);

        // ProductAssetDetailsPanel.ui:139-143 (Disabled, WindowText). TOKENIZE: ProductPanelSeparatorLineDisabled
        inline constexpr QColor SeparatorLineDisabled(120, 120, 120, 255);

        // ProductAssetDetailsPanel.ui:72-76, :81-85, :90-94 (Active Button, Base, Window), :110-114, :119-123, :128-132 (Inactive),
        // :148-152, :157-161, :166-170 (Disabled). TOKENIZE: ProductPanelSeparatorFill
        inline constexpr QColor SeparatorFill(87, 87, 87, 255);

        //! The palette of the "AssetDetailsSeparator" line, set right after setupUi
        inline QPalette SeparatorPalette()
        {
            return MakeSeparatorPalette(SeparatorLineActive, SeparatorLineInactive, SeparatorLineDisabled, SeparatorFill);
        }
    } // namespace ProductAssetDetailsPanelFlat

    // ------------------------------------------------------------------------------
    // SourceAssetDetailsPanel: SourceAssetDetailsPanel.ui:58-175 (the line's <palette>); the intermediate assets panel is the same class
    // ------------------------------------------------------------------------------

    namespace SourceAssetDetailsPanelFlat
    {
        // SourceAssetDetailsPanel.ui:63-67 (Active, WindowText). TOKENIZE: SourcePanelSeparatorLineActive
        inline constexpr QColor SeparatorLineActive(200, 200, 200, 200);

        // SourceAssetDetailsPanel.ui:101-105 (Inactive, WindowText). TOKENIZE: SourcePanelSeparatorLineInactive
        inline constexpr QColor SeparatorLineInactive(255, 255, 255, 255);

        // SourceAssetDetailsPanel.ui:139-143 (Disabled, WindowText). TOKENIZE: SourcePanelSeparatorLineDisabled
        inline constexpr QColor SeparatorLineDisabled(120, 120, 120, 255);

        // SourceAssetDetailsPanel.ui:72-76, :81-85, :90-94 (Active Button, Base, Window), :110-114, :119-123, :128-132 (Inactive),
        // :148-152, :157-161, :166-170 (Disabled). TOKENIZE: SourcePanelSeparatorFill
        inline constexpr QColor SeparatorFill(87, 87, 87, 255);

        //! The palette of the "AssetDetailsSeparator" line, set right after setupUi
        inline QPalette SeparatorPalette()
        {
            return MakeSeparatorPalette(SeparatorLineActive, SeparatorLineInactive, SeparatorLineDisabled, SeparatorFill);
        }
    } // namespace SourceAssetDetailsPanelFlat
} // namespace AssetProcessor

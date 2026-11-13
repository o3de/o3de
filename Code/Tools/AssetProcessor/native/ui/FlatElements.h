/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

class QWidget;

namespace AssetProcessor
{
    //! The flattened GUI (AzQtComponents::StyleManager::stylesheetsDisabled) carries AssetProcessor.qss and the sheets it imports
    //! (AssetsTab.qss, LogsTab.qss; NewLogTabDialog.qss, AzToolsFramework's, only says what '*' says) as elements of the
    //! AzQtComponents registries: text, frame and box elements, each the final look of the widgets its matcher picks.
    //! StyleManager::registerFlatSheet runs this once, the first time the sheet is set on a widget; the legacy GUI never calls it.
    //! The registries last for the process, so a second call does nothing.
    void RegisterFlatElements();

    //! The inline sheets of the Asset Processor's windows: those its .ui forms gave their widgets and those its code sets. The legacy
    //! GUI sets them (SetLegacySheet); the flattened GUI sets none, and the elements RegisterFlatElements registers draw their look
    //! (FlatElements.cpp, "The inline sheets"). Each text is the one the form or the call had before the flattening.
    namespace InlineSheets
    {
        // ---- The forms: MainWindow.ui, ProductAssetDetailsPanel.ui, SourceAssetDetailsPanel.ui
        inline constexpr const char* Divider = "background-color: #616161;";            //!< the dividers of the settings page
        inline constexpr const char* PanelFill = "background-color: rgb(87, 87, 87);";  //!< the detail panels
        inline constexpr const char* BoldFont = "font: bold;";
        inline constexpr const char* PointFont = "font: 12pt;";
        inline constexpr const char* BoldPointFont = "font: bold, 12pt;";
        inline constexpr const char* NoPadding = "padding:0px";

        // ---- The code: MainWindow.cpp
        inline constexpr const char* RemovePatternButton = "QPushButton { background-color: transparent; border: 0px }";
        inline constexpr const char* SharedCacheStatus = "QLabel#sharedCacheStatus";
    } // namespace InlineSheets

    //! Sets one of the InlineSheets on the widget, in the legacy GUI only (AzQtComponents::StyleManager::setLegacyStyleSheet): the
    //! flattened GUI never makes the sheet, so the widget never gets a QStyleSheetStyle
    void SetLegacySheet(QWidget* widget, const char* sheet);
} // namespace AssetProcessor

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QColor>
#include <QMargins>
#endif

namespace AzQtComponents
{
    //! The foundations of the flattened GUI: the values many families stand on, each defined once. A rule of the legacy sheets that
    //! reached many families lives here, and a family uses it from here; a family's own value that sits on a primitive layer keeps
    //! its own name and token id and takes its value from the layer. A theme that shifts a foundation reaches every feature on it.
    namespace Foundations
    {
        // ------------------------------------------------------------------------------
        // Primitive layers: the background tiers of the BJDS grey ladder, darkest first, and no paint
        // ------------------------------------------------------------------------------

        inline constexpr QColor FarBG(0x11, 0x11, 0x11);           // BJDS Gray Far BG $tier.FarBG
        inline constexpr QColor BG(0x22, 0x22, 0x22);              // BJDS Gray BG $tier.BG
        inline constexpr QColor Low(0x33, 0x33, 0x33);             // BJDS Gray Low $tier.Low
        inline constexpr QColor Mid(0x44, 0x44, 0x44);             // BJDS Gray Mid $tier.Mid
        inline constexpr QColor Highest(0x55, 0x55, 0x55);         // BJDS Gray Highest $tier.Highest
        inline constexpr QColor Clear(0x00, 0x00, 0x00, 0x00);     // Qt::transparent $tier.Clear

        // ------------------------------------------------------------------------------
        // The universal rule: BaseStyleSheet.qss's `*`, which reached every widget
        // ------------------------------------------------------------------------------

        namespace Surface
        {
            inline constexpr QColor Fill = Clear;                  // BaseStyleSheet.qss:17 $fill.surface.idle.Clear
            inline constexpr QColor Text(0xFF, 0xFF, 0xFF);        // BaseStyleSheet.qss:18 $text.surface.idle.Alt1
            inline constexpr const char* Family = "Open Sans";     // BaseStyleSheet.qss:19 $type.surface.idle.family
            inline constexpr int Weight = 400;                     // BaseStyleSheet.qss:20 $type.surface.idle.weight
            //! StyleManager::initialize (the application font), ColorPicker.qss:18 (`ColorPicker *`), Text.qss:92 (`.Label`)
            //! $type.surface.idle.size
            inline constexpr int Size = 12;
        } // namespace Surface

        // ------------------------------------------------------------------------------
        // The label rule: Text.qss's `.Label, QLabel`; the tiers of Text.qss and TitleBar.qss restate its family and size
        // ------------------------------------------------------------------------------

        namespace Label
        {
            //! Text.qss:91 (QLabel), :57, :66, :74, :83, :99, :114, :122 (the tiers), TitleBar.qss:33 $type.label.idle.family.Alt1
            inline constexpr const char* Family = "Open Sans";
            inline constexpr int Size = 12;                        // Text.qss:92, :84, :100, :115, :123 $type.label.idle.size.Alt1
            //! Text.qss:93-94 $space.label.idle.margin-top.Alt1 $space.label.idle.margin-bottom.Alt1
            inline constexpr int MarginV = 1;
        } // namespace Label

        // ------------------------------------------------------------------------------
        // The entry field: BaseStyleSheet.qss's rule of the entry controls (QTextEdit, QPlainTextEdit, QLineEdit, QSpinBox,
        // QDoubleSpinBox, QComboBox), and what LineEdit.qss, ComboBox.qss and SpinBox.qss share
        // ------------------------------------------------------------------------------

        namespace Field
        {
            inline constexpr QColor Fill(0xCC, 0xCC, 0xCC);        // BaseStyleSheet.qss:76, ComboBox.qss:19 $fill.field.idle.Alt1
            inline constexpr QColor Text(0x00, 0x00, 0x00);        // BaseStyleSheet.qss:77 $text.field.idle.Alt1
            inline constexpr const char* Family = "Open Sans";     // BaseStyleSheet.qss:78 $type.field.idle.family
            inline constexpr int Radius = 2;                       // BaseStyleSheet.qss:79 $radius.field.idle.all
            inline constexpr QColor Line(0xE9, 0xE9, 0xE9);        // BaseStyleSheet.qss:81 $line.field.idle.color.Alt1
            inline constexpr int Size = 12;                        // BaseStyleSheet.qss:82 $type.field.idle.size.Alt1
            //! LineEdit.qss:13, ComboBox.qss:15, SpinBox.qss:12, BrowseEdit.qss:17 $size.field.idle.height.Alt1
            inline constexpr int Height = 16;
            inline constexpr QMargins Margin(2, 2, 2, 2);          // SpinBox.qss:13, BrowseEdit.qss:12 $space.field.idle.margin.Alt1
            inline constexpr int PaddingLeft = 0;                  // LineEdit.qss:15, SpinBox.qss:17 $space.field.idle.padding-left.Alt1
            inline constexpr int PaddingTop = -1;                  // LineEdit.qss:19, SpinBox.qss:19 $space.field.idle.padding-top.Alt1
            inline constexpr int PaddingBottom = 1;                // LineEdit.qss:18, SpinBox.qss:18 $space.field.idle.padding-bottom

            // ---- Hovered, focused, in error, disabled
            //! ComboBox.qss:25, SpinBox.qss:28, BaseStyleSheet.qss:102 $line.field.hover.width.Alt1
            inline constexpr int HoverLineWidth = 1;
            inline constexpr QMargins HoverMargin(1, 1, 1, 1);     // SpinBox.qss:27, BaseStyleSheet.qss:101 $space.field.hover.margin.Alt1
            inline constexpr QColor HoverLine = BG;                // SpinBox.qss:36, BaseStyleSheet.qss:104 $line.field.hover.color.BG
            inline constexpr int HoverRadius = 3;                  // ComboBox.qss:27, SpinBox.qss:30 $radius.field.hover.all
            //! ComboBox.qss:36, SpinBox.qss:28, BaseStyleSheet.qss:112 $line.field.focus.width.Alt1
            inline constexpr int FocusLineWidth = 1;
            inline constexpr QMargins FocusMargin(1, 1, 1, 1);     // SpinBox.qss:27, BaseStyleSheet.qss:111 $space.field.focus.margin.Alt1
            inline constexpr int FocusRadius = 3;                  // ComboBox.qss:38, SpinBox.qss:30 $radius.field.focus.all
            //! ComboBox.qss:39, SpinBox.qss:45, BaseStyleSheet.qss:113 $line.field.focus.color.Alt1
            inline constexpr QColor FocusLine(0x94, 0xD2, 0xFF);
            //! ComboBox.qss:41, SpinBox.qss:46, BrowseEdit.qss:41 $fill.field.focus.Alt1
            inline constexpr QColor FocusFill(0xFF, 0xFF, 0xFF);
            inline constexpr QMargins ErrorMargin(0, 0, 0, 0);     // ComboBox.qss:46, SpinBox.qss:62 $space.field.HasError
            inline constexpr int ErrorLineWidth = 2;               // ComboBox.qss:47, SpinBox.qss:63 $line.field.HasError.width
            inline constexpr int ErrorRadius = 4;                  // ComboBox.qss:49, SpinBox.qss:65 $radius.field.HasError
            inline constexpr QColor ErrorLine(0xE2, 0x52, 0x43);   // ComboBox.qss:50, SpinBox.qss:66 $line.field.HasError.color
            //! LineEdit.qss:33, ComboBox.qss:55, SpinBox.qss:72 $fill.field.disabled
            inline constexpr QColor DisabledFill(0x66, 0x66, 0x66);
            //! LineEdit.qss:34, ComboBox.qss:56, SpinBox.qss:73 $text.field.disabled.Alt1
            inline constexpr QColor DisabledText(0x99, 0x99, 0x99);
        } // namespace Field
    } // namespace Foundations
} // namespace AzQtComponents

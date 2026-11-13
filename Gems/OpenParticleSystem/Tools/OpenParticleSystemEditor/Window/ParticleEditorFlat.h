/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QColor>
#include <QString>

class QLineEdit;
class QWidget;

// ----------------------------------------------------------------------------------
// The flattened GUI's home of the Particle Editor (the "(Preview) Particle Editor" view pane): the values its widgets drew with, the
// elements that carry them (ParticleEditorFlat.cpp), and the sheets the legacy GUI sets for the same look. The editor has no style
// sheet of its own that anything loads (OpenParticleEditor.qss is compiled into the resources and read by nothing), so its widgets
// gave themselves sheets, inline and in their forms. The legacy GUI sets them (StyleManager::setLegacyStyleSheet), written from the
// values below; the flattened GUI sets none, and the elements give the widgets the same look.
// ----------------------------------------------------------------------------------

namespace OpenParticleSystemEditor
{
    //=========================================================================
    // VALUES (one owner for each: the sites and the elements read them here)
    //=========================================================================

    //=========================================================================
    // EFFECTOR INSPECTOR COLOURS (the particle name edit: idle, focused and blurred; the icon beside it)
    //=========================================================================
    namespace EffectorInspectorFlat
    {
        constexpr QColor NameEditIdleFill(0, 0, 0, 0);    // EffectorInspector.cpp:31 (transparent). TOKENIZE: ParticleNameEditIdleFill
        constexpr QColor NameEditText(255, 255, 255);     // EffectorInspector.cpp:31, :58. TOKENIZE: ParticleNameEditText
        constexpr QColor NameEditFocusedText(68, 68, 68); // EffectorInspector.cpp:54. TOKENIZE: ParticleNameEditFocusedText
        constexpr QColor NameEditBlurredFill(68, 68, 68); // EffectorInspector.cpp:58. TOKENIZE: ParticleNameEditBlurredFill
        constexpr QColor NameIconTop(223, 169, 112);      // EffectorInspector.ui:46 `stop:0`. TOKENIZE: ParticleNameIconTop
        constexpr QColor NameIconBottom(217, 141, 50);    // EffectorInspector.ui:46 `stop:1`. TOKENIZE: ParticleNameIconBottom
    } // namespace EffectorInspectorFlat

    //=========================================================================
    // PARTICLE GRAPHICS VIEW COLOURS (the fill behind the emitter items)
    //=========================================================================
    namespace ParticleGraphicsViewFlat
    {
        constexpr QColor ViewFill(0x2D, 0x2D, 0x2D); // ParticleGraphicsView.cpp:52. TOKENIZE: ParticleGraphicsViewFill
    } // namespace ParticleGraphicsViewFlat

    //=========================================================================
    // PARTICLE ITEM WIDGET COLOURS (the fill of the emitter window; the title row, the selection lines and the check boxes of its form)
    //=========================================================================
    namespace ParticleItemWidgetFlat
    {
        constexpr QColor ItemWindowFill(0x33, 0x33, 0x33); // ParticleItemWidget.cpp:54. TOKENIZE: ParticleItemWindowFill
        constexpr int ItemWindowRadius = 8;                // ParticleItemWidget.cpp:54. TOKENIZE: ParticleItemWindowRadius
        constexpr QColor TitleTop(223, 169, 112);          // ParticleItemWidget.ui:59 `stop:0`. TOKENIZE: ParticleItemTitleTop
        constexpr QColor TitleBottom(217, 141, 50);        // ParticleItemWidget.ui:59 `stop:1`. TOKENIZE: ParticleItemTitleBottom
        constexpr int TitleRadius = 8;                     // ParticleItemWidget.ui:60-61 top. TOKENIZE: ParticleItemTitleRadius
        constexpr int TitleRowRadius = 1;                  // ParticleItemWidget.ui:62-63 bottom. TOKENIZE: ParticleItemTitleRowRadius
        // ParticleItemWidget.ui:781, :797, :813, :829 `yellow`. TOKENIZE: ParticleItemSelectionLine
        constexpr QColor SelectionLine(255, 255, 0);

        // The images of the check boxes: one image for every state of the box (the widget's sheet outranks the application's
        // :focus and :disabled rules for the images, CheckBox.qss:50-73)
        // ParticleItemWidget.ui:120. TOKENIZE: ParticleItemSoloUncheckedImage
        constexpr const char* SoloUncheckedImage = ":/Gallery/Asset_File.svg";
        // ParticleItemWidget.ui:121. TOKENIZE: ParticleItemSoloCheckedImage
        constexpr const char* SoloCheckedImage = ":/Gallery/Favorite_File.svg";
        // ParticleItemWidget.ui:141, PropertyEditorWidget.cpp:55. TOKENIZE: ParticleItemFixedUncheckedImage
        constexpr const char* FixedUncheckedImage = ":/stylesheet/img/UI20/checkbox/off.svg";
        // ParticleItemWidget.ui:142, PropertyEditorWidget.cpp:56. TOKENIZE: ParticleItemFixedCheckedImage
        constexpr const char* FixedCheckedImage = ":/stylesheet/img/UI20/checkbox/on.svg";
    } // namespace ParticleItemWidgetFlat

    //=========================================================================
    // PARTICLE LINE WIDGET COLOURS (the fill of a row: idle and pressed)
    //=========================================================================
    namespace ParticleLineWidgetFlat
    {
        constexpr QColor LineIdleFill(0x47, 0x47, 0x47);    // ParticleLineWidget.cpp:18, :55. TOKENIZE: ParticleLineIdleFill
        constexpr QColor LinePressedFill(0x66, 0x66, 0x66); // ParticleLineWidget.cpp:40. TOKENIZE: ParticleLinePressedFill
        constexpr int LineRadius = 1;                       // ParticleLineWidget.cpp:18, :40, :55. TOKENIZE: ParticleLineRadius
    } // namespace ParticleLineWidgetFlat

    //=========================================================================
    // THE SITES' SIDE (registration, the sheets of the legacy GUI, the states)
    //=========================================================================
    namespace ParticleEditorFlat
    {
        //! The look of the particle name edit: before it has had the focus, while it has it, and after it lost it
        enum class NameEditState
        {
            Initial,
            Focused,
            Blurred,
        };

        // ---- Registration
        //! The flattened GUI only: registers the elements of the editor's widgets, once. The first window made registers them, before it
        //! builds its widgets; a session that never opens the pane pays nothing
        void RegisterFlatElements();

        // ---- The sheets the legacy GUI sets (the flattened GUI sets none)
        QString NameIconSheet();                //!< EffectorInspector.ui:46, the icon beside the name
        QString TitleRowSheet();                //!< ParticleItemWidget.ui:59-63, the title row
        QString LineRowSheet(bool pressed);     //!< ParticleLineWidget.cpp:18, :40, :55, a row idle and pressed
        QString FixedCheckSheet();              //!< ParticleItemWidget.ui:141-142, PropertyEditorWidget.cpp:55-56
        QString SoloCheckSheet();               //!< ParticleItemWidget.ui:120-121
        QString SelectionLineSheet();           //!< ParticleItemWidget.ui:781, :797, :813, :829

        // ---- The states
        //! A row is pressed or let go. The legacy GUI sets the row's sheet; the flattened GUI tags the row (Style::addClass, which
        //! polishes it) and polishes its tool buttons again, which read the tag of the row they sit in
        void SetLineRowPressed(QWidget* row, bool pressed);

        //! The name edit has the look of the state. The legacy GUI sets the edit's sheet; the flattened GUI tags the edit
        void SetNameEditState(QLineEdit* edit, NameEditState state);
    } // namespace ParticleEditorFlat
} // namespace OpenParticleSystemEditor

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QString>
#include <QWidget>

namespace EMStudioFlat
{
    //! Flattened GUI only (AzQtComponents::StyleManager::stylesheetsDisabled): the elements that carry what the inline style sheets of
    //! the Animation Editor's widgets gave them (EMotion Studio, the anim graph editor, the simulated object widgets and MysticQt's
    //! dialog stack). The legacy GUI sets those sheets (StyleManager::setLegacyStyleSheet); the flattened GUI sets none, and these
    //! elements draw what the sheets drew. EMStudioManager's constructor registers this function for the sheet of the Editor
    //! (StyleManager::registerFlatSheet), and the flattened GUI runs it when that sheet is first set; legacy mode never does.
    void RegisterElements();

    // ----------------------------------------------------------------------------------
    // The object names the sites give their widgets and the matchers test first. A sheet named none of these: the legacy GUI ignores
    // them. A widget of the Animation Editor that had a name already keeps it (the motion set combo box).
    // ----------------------------------------------------------------------------------

    inline constexpr const char* InputFieldName = "EMFXInputField";             //!< the name fields of the rename dialogs, the track name
    inline constexpr const char* ShortcutLabelName = "EMFXShortcutLabel";       //!< the two labels of the keyboard shortcut dialog
    inline constexpr const char* RecoverCheckName = "EMFXRecoverCheck";         //!< the check box of a file of the recover files dialog
    inline constexpr const char* DialogTopLabelName = "EMFXDialogTopLabel";     //!< the title strip of the reset workspace dialog
    inline constexpr const char* BoldLabelName = "EMFXBoldLabel";               //!< the labels of a bold sheet (`font-weight: bold;`)
    //! The motion set combo box of the anim graph editor, named by AnimGraphEditor::UpdateMotionSetComboBox (a UI test finds it)
    inline constexpr const char* MotionSetComboName = "EMFX.AttributesWindowWidget.AnimGraph.MotionSetComboBox";
    inline constexpr const char* GraphTreeName = "EMFXGraphTree";               //!< the trees of the hierarchy and navigate widgets
    inline constexpr const char* ProgressTextName = "EMFXProgressText";         //!< the progress text of the command bar
    inline constexpr const char* ProgressBarName = "EMFXProgressBar";           //!< the progress bar of the command bar
    inline constexpr const char* ResultEditName = "EMFXResultEdit";             //!< the result field of the command bar
    inline constexpr const char* CaretCheckName = "EMFXCaretCheck";             //!< the 'Motion Extraction' check box
    inline constexpr const char* AxisXName = "EMFXAxisX";                       //!< the X label of a blend space motion's position
    inline constexpr const char* AxisYName = "EMFXAxisY";                       //!< the Y label

    // ----------------------------------------------------------------------------------
    // The class tags of the states (Style::addClass), set in the flattened GUI only: the legacy GUI's state is the sheet itself
    // ----------------------------------------------------------------------------------

    inline constexpr const char* ConflictTag = "EMFXConflict";                  //!< a shortcut label while the key is taken
    inline constexpr const char* InvalidInputTag = "EMFXInvalidInput";          //!< an input field while its text is not valid
    inline constexpr const char* DisabledTrackTag = "EMFXDisabledTrack";        //!< the name field of a disabled time track
    inline constexpr const char* MotionSetWarningTag = "EMFXWarning";           //!< the motion set combo box while it warns
    inline constexpr const char* ResultErrorTag = "EMFXResultError";            //!< the result field after a command failed
    inline constexpr const char* ResultSuccessTag = "EMFXResultSuccess";        //!< the result field after a command succeeded
    inline constexpr const char* NotificationHeaderTag = "EMFXNotificationHeader";  //!< the header frame of a notification widget
    inline constexpr const char* InvalidMotionTag = "EMFXInvalidMotion";        //!< the motion label of a blend space while it is invalid

    //! The class tag of a state, in the flattened GUI only: the tag is there while the state holds, and the widget is polished again
    //! when it changes (Style::addClass, Style::removeClass; a tag the widget has, or lacks, already changes nothing). Beside the
    //! StyleManager::setLegacyStyleSheet of the site, which is the state of the legacy GUI.
    inline void SetStateTag(QWidget* widget, const char* tag, bool on)
    {
        if (!AzQtComponents::StyleManager::flatStylingActive())
        {
            return;
        }
        if (on)
        {
            AzQtComponents::Style::addClass(widget, QString::fromLatin1(tag));
        }
        else
        {
            AzQtComponents::Style::removeClass(widget, QString::fromLatin1(tag));
        }
    }
} // namespace EMStudioFlat

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QMetaObject>

namespace AZ { class Vector2; }

class QComboBox;
class QLineEdit;
class QLabel;
class EditorWindow;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! CanvasSizeToolbar provides controls to configure the canvas size
class CanvasSizeToolbarSection
{
public: // methods

    explicit CanvasSizeToolbarSection(QToolBar* parent);
    virtual ~CanvasSizeToolbarSection();

    void InitWidgets(QToolBar* parent, bool addSeparator);

    //! Given a canvas set, select a preset or automatically populate the custom canvas size text boxes
    //!
    //! This method is called when loading a canvas
    void SetInitialResolution(const AZ::Vector2& canvasSize);

    //! Change the combo box index. Called by undo/redo commands
    void SetIndex(int index);

    //! Change the combo box index to that of the specified canvas size. Called by undo/redo commands
    void SetCustomCanvasSize(AZ::Vector2 canvasSize, bool findPreset);

    const QString& IndexToString(int index);

protected: // types

    //! Simple encapsulation of canvas size width and height presets, along with descriptions.
    struct CanvasSizePresets
    {
        CanvasSizePresets(const QString& desc, int width, int height)
            : description(desc)
            , width(width)
            , height(height) { }

        QString description;
        int width;
        int height;
    };

protected: // methods

    //! Updates the state of the custom canvas size fields based on the currently selected index.
    //
    //! When "other..." is selected, the fields become visible and take focus. Otherwise, this
    //! method hides the fields (as they are not needed when using preset canvas sizes).
    void ToggleLineEditBoxes();

    //! Sets the canvas size based on the current canvas ComboBox selection
    virtual void SetCanvasSizeByComboBoxIndex() = 0;

    //! Called when the user has changed the index
    virtual void OnComboBoxIndexChanged(int index) = 0;

    //! Add any special entries in the combo box
    virtual void AddSpecialPresets() {}

    //! Attempts to parse the canvas size presets JSON
    //
    //! \return True if successful, false otherwise
    bool ParseCanvasSizePresetsJson(const QJsonDocument& jsonDoc);

    //! Initializes canvas size presets options via JSON file
    //
    //! Falls back on hard-coded defaults if JSON file parsing fails.
    void InitCanvasSizePresets();

    //! Return a preset index based on specified canvas size
    int GetPresetIndexFromSize(AZ::Vector2 size);

    //! Called when the user is finished entering text for custom canvas width size.
    void LineEditWidthEditingFinished();

    //! Called when the user is finished entering text for custom canvas height size.
    void LineEditHeightEditingFinished();

    //! Handle updates after index has changed
    void HandleIndexChanged();

    int GetCustomSizeIndex();

protected: // members

    EditorWindow* m_editorWindow;

    int m_comboIndex;

    bool m_isChangeUndoable;

    // Canvas size presets
    typedef AZStd::vector<CanvasSizePresets> ComboBoxOptions;
    ComboBoxOptions m_canvasSizePresets;
    QComboBox* m_combobox;

    // Custom canvas size
    QLineEdit* m_lineEditCanvasWidth;
    QLabel* m_labelCustomSizeDelimiter;
    QLineEdit* m_lineEditCanvasHeight;

    QMetaObject::Connection m_lineEditCanvasWidthConnection;
    QMetaObject::Connection m_lineEditCanvasHeightConnection;

    // The toolbar actions for custom canvas size are exclusively used for controlling
    // the visibility of the widgets within the toolbar.
    QAction* m_canvasWidthAction;
    QAction* m_canvasDelimiterAction;
    QAction* m_canvasHeightAction;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! ReferenceCanvasSizeToolbarSection provides controls to configure the reference canvas size
//! (a.k.a. authored canvas size) or the canvas being edited
class ReferenceCanvasSizeToolbarSection
    : public CanvasSizeToolbarSection
{
public: // methods

    explicit ReferenceCanvasSizeToolbarSection(QToolBar* parent, bool addSeparator = false);

private: // methods

    //! Sets the canvas size based on the current canvas ComboBox selection
    void SetCanvasSizeByComboBoxIndex() override;

    void OnComboBoxIndexChanged(int index) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! PreviewCanvasSizeToolbarSection provides controls to configure the preview canvas size
class PreviewCanvasSizeToolbarSection
    : public CanvasSizeToolbarSection
{
public: // methods

    explicit PreviewCanvasSizeToolbarSection(QToolBar* parent, bool addSeparator = false);

private: // methods

    //! Sets the canvas size based on the current canvas ComboBox selection
    void SetCanvasSizeByComboBoxIndex() override;

    void OnComboBoxIndexChanged(int index) override;

    void AddSpecialPresets() override;
};

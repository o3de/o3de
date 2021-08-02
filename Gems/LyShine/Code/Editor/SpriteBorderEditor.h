/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//! \brief Visual sprite editor used to configure slicing and sprite-sheet properties for a given sprite.
class SpriteBorderEditor
    : public QDialog
{
    Q_OBJECT

public:

    SpriteBorderEditor(const char* path, QWidget* parent = nullptr);

    ~SpriteBorderEditor();

    bool GetHasBeenInitializedProperly();

signals:

    //! Signals when a new cell within the sprite-sheet has been selected.
    void SelectedCellChanged(ISprite* sprite, AZ::u32 index = 0);

    //! Signals when the Sprite Editor UI is being reset.
    //!
    //! See ResetUi.
    void ResettingUi();

private: // types

    //! Sprite info required to restore sprite to its original state if user cancels the dialog.
    struct SpritesheetRestoreInfo
    {
        ISprite::SpriteSheetCellContainer spriteSheetCells;
        ISprite::Borders borders;
    };

private: // functions

    //! Reconstructs the UI widgets from sprite and member data.
    void ResetUi();

    //! Removes all widgets from the dialog.
    void ClearLayout();

    //! Re-calculates sprite-sheet cell UV info and resets the UI.
    void UpdateSpriteSheetCellInfo(int newNumRows, int newNumCols, ISprite* sprite);

    //! Given the cell index, updates the properties view with the selected sprite-sheet cell.
    void DisplaySelectedCell(AZ::u32 cellIndex);

    void AddConfigureSection(QGridLayout* gridLayout, int& rowNum);
    void AddSelectCellSection(QGridLayout* gridLayout, int& rowNum);
    void AddPropertiesSection(QGridLayout* gridLayout, int& rowNum);
    void AddButtonsSection(QGridLayout* gridLayout, int& rowNum);
    void AddSeparator(QGridLayout* gridLayout, int& rowNum);
    void SetDisplayedTextureSize(float width, float height);
    bool IsConfiguringSpriteSheet() const;

private slots:

    //! Creates the window layout, populating the dialog with widget content.
    void CreateLayout();

private: // member vars

    SpritesheetRestoreInfo m_restoreInfo;                       //!< Stores starting sprite configuration in case the user cancels the dialog

    static const int numSliceBorders = 4;
    SlicerManipulator* m_manipulators[numSliceBorders];

    QString m_spritePath;                                       //!< Path to the sprite being edited.
    QPixmap m_unscaledSpriteSheet;                              //!< Unscaled sprite-sheet image, useful for determining cell locations by UV coords.
    QGraphicsPixmapItem* m_cellPropertiesPixmap     = nullptr;  //!< Image item data used by the graphics scene to display image/cell.
    QGraphicsScene* m_cellPropertiesGraphicsScene   = nullptr;  //!< Image/sprite-sheet cell "view" displayed in window
    QLabel* m_textureSizeLabel                      = nullptr;  //!< Displays the selected texture/cell size info.
    QLineEdit* m_cellAliasLineEdit                  = nullptr;  //!< Displays the selected cell alias.
    QWidget* m_prevTabField                         = nullptr;  //!< Points to the previous field that should be tabbed from (see calls to QWidget::setTabOrder).
    uint m_numRows                                  = 1;        //!< Number of rows in spritesheet cell grid.
    uint m_numCols                                  = 1;        //!< Number of cols in spritesheet cell grid.
    bool m_hasBeenInitializedProperly               = true;     //!< True if dialog was constructed properly, false otherwise
    ISprite* m_sprite                               = nullptr;  //!< Currently loaded sprite.
    AZ::u32 m_currentCellIndex                      = 0;        //!< Currently selected cell index
    bool m_configureAsSpriteSheet                   = false;    //!< Forces the spritesheet configuration section to display
    bool m_resizeOnce                               = true;     //!< Resize the window only once (since CreateLayout is called multiple times)
};

//! \brief A custom rect item that allows us to get a mouse press event
//!
//! This provides a convenient callback to determine which cell index
//! was selected within the cell selection view.
class CellSelectRectItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    CellSelectRectItem(const QRectF& rect, const AZStd::function<void()>& clickCallback);

    ~CellSelectRectItem() override;

    static void ClearSelection() { s_currentSelection = nullptr; }

    //! Activates the "selected cell" border styling.
    //!
    //! Changes the QBrush styling for this RectItem to draw the
    //! "selected" border style and de-selects the previously
    //! selected item by removing the brush styling from it.
    void SelectCell();

public slots:

    void StopProcessingInput()
    {
        m_processInput = false;
    }

protected:

    //! "Selects" the cell and triggers the associated click callback.
    void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

private:
    static CellSelectRectItem* s_currentSelection;

    //! Function to call when this cell/rectitem is selected.
    AZStd::function<void()> m_clickCallback;

    //! Determines whether input events are processed or not.
    //!
    //! It's useful to turn off further event processing such as when
    //! the Sprite Editor UI is being reset to apply a new configuration
    //! or layout.
    bool m_processInput = true;
};

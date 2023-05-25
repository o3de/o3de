/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SpriteBorderEditorCommon.h"

#include <AzCore/Interface/Interface.h>
#include <QMessageBox>
#include <QApplication>
#include <Util/PathUtil.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
//-------------------------------------------------------------------------------

namespace
{
    // Various pixel values that affect layout and appearance of items within
    // the Sprite Editor dialog.
    const int sectionContentLeftMargin = 24;
    const int sectionContentTopMargin = sectionContentLeftMargin / 2;
    const int sectionContentBottomMargin = sectionContentLeftMargin / 2;
    const int interElementSpacing = 16;
    const int textInputWidth = 100;
}

CellSelectRectItem* CellSelectRectItem::s_currentSelection = nullptr;

SpriteBorderEditor::SpriteBorderEditor(const char* path, QWidget* parent)
    : QDialog(parent)
    , m_spritePath(path)
{
    memset(m_manipulators, 0, sizeof(m_manipulators));

    // Remove the ability to resize this window.
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);

    // Make sure the sprite can load and be displayed before continuing
    m_sprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePath.toLatin1().constData());
    QString fullPath = Path::GamePathToFullPath((m_sprite->GetTexturePathname() + "." + AZ::RPI::StreamingImageAsset::Extension).c_str());
    const bool imageWontLoad = !m_sprite || QPixmap(fullPath).isNull();
    if (imageWontLoad)
    {
        m_hasBeenInitializedProperly = false;
        return;
    }

    // Store a copy of the sprite-sheet's current configuration in case the
    // user decides to cancel this dialog
    m_restoreInfo.spriteSheetCells = m_sprite->GetSpriteSheetCells();
    m_restoreInfo.borders = m_sprite->GetBorders();

    CreateLayout();

    setWindowTitle("Sprite Editor");
    setModal(true);
    setWindowModality(Qt::ApplicationModal);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    // Position the widget centered around cursor.
    {
        QSize halfSize = (layout()->sizeHint() / 2);
        move(QCursor::pos() - QPoint(halfSize.width(), halfSize.height()));
    }

    // Set the "configure as sprite-sheet" flag if we start with a 
    // sprite-sheet. This resolves a bug where we start with a sprite-sheet
    // and the user decides to change the row and col to 1x1 which results
    // in the dialog having the sprite-sheet sections removed, but still
    // retain the original dialog size. If the user "removes" the sprite-sheet
    // data by setting row and col to 1x1, then they'll see the basic dialog
    // next time they open the editor.
    if (m_sprite->IsSpriteSheet())
    {
        m_configureAsSpriteSheet = true;
        m_resizeOnce = false;
    }
}

SpriteBorderEditor::~SpriteBorderEditor()
{
    m_sprite->Release();
}

bool SpriteBorderEditor::GetHasBeenInitializedProperly()
{
    return m_hasBeenInitializedProperly;
}

void SpriteBorderEditor::CreateLayout()
{
    // The layout.
    QGridLayout* outerGrid = new QGridLayout(this);

    QGridLayout* innerGrid = new QGridLayout();
    outerGrid->addLayout(innerGrid, 0, 0, 1, 2);

    int layoutRowIncrement = 0;

    if (IsConfiguringSpriteSheet())
    {
        AddConfigureSection(innerGrid, layoutRowIncrement);
        AddSeparator(innerGrid, layoutRowIncrement);

        AddSelectCellSection(innerGrid, layoutRowIncrement);
        AddSeparator(innerGrid, layoutRowIncrement);
    }

    AddPropertiesSection(innerGrid, layoutRowIncrement);
    AddButtonsSection(outerGrid, layoutRowIncrement);

    // If dialog is closed without saving, restore original border values.
    {
        ISprite::Borders originalBorders = m_sprite->GetBorders();
        QObject::connect(this,
            &QDialog::rejected,
            this,
            [this, originalBorders]()
        {
            // Restore original borders.
            m_sprite->SetBorders(originalBorders);
        });
    }

    // Default to displaying the first cell of the spritesheet
    static const int firstCellIndex = 0;
    DisplaySelectedCell(firstCellIndex);

    // CreateLayout can be called multiple times, so make sure we only resize
    // the window once.
    if (m_configureAsSpriteSheet && m_resizeOnce)
    {
        m_resizeOnce = false;

        // Scale the height and width of the window to account for the 
        // additional space required by the spritesheet configuration 
        // sections. Probably the "correct" way to solve this would be 
        // dynamically recreating (or somehow updating) the QLayout of the
        // window.
        const float heightScale = 2.15f;
        const float widthScale = 1.15f;
        QSize currentSize(size());
        
        setFixedSize(aznumeric_cast<int>(currentSize.width() * widthScale), aznumeric_cast<int>(currentSize.height() * heightScale));
    }
}

void SpriteBorderEditor::ResetUi()
{
    emit ResettingUi();

    // Disconnect all objects from the sprite editor's signals
    disconnect();

    ClearLayout();

    // Repopulate the window contents on the next Qt event loop tick.
    QMetaObject::invokeMethod(this, "CreateLayout", Qt::QueuedConnection);
}

void SpriteBorderEditor::ClearLayout()
{
    // Remove all children from the dialog
    auto childWidgets = children();
    for (auto childWidget : childWidgets)
    {
        // We deleteLater in case this window still has events sitting on
        // the event queue for this particular tick of the Qt event loop.
        childWidget->deleteLater();
    }

    m_cellPropertiesPixmap = nullptr;
    m_cellPropertiesGraphicsScene = nullptr;
    m_textureSizeLabel = nullptr;
    m_cellAliasLineEdit = nullptr;

    CellSelectRectItem::ClearSelection();
}

void SpriteBorderEditor::UpdateSpriteSheetCellInfo(int newNumRows, int newNumCols, ISprite* sprite)
{
    // Because the row/column sprite-sheet configuration is changing, we need
    // to remove the current sprite-sheet configuration for this sprite.
    sprite->ClearSpriteSheetCells();

    m_numRows = newNumRows;
    m_numCols = newNumCols;

    float floatNumRows = aznumeric_cast<float>(m_numRows);
    float floatNumCols = aznumeric_cast<float>(m_numCols);

    // Calculate uniformly sized sprite-sheet cell UVs based on the given
    // row and column cell configuration.
    for (unsigned int row = 0; row < m_numRows; ++row)
    {
        for (unsigned int col = 0; col < m_numCols; ++col)
        {
            AZ::Vector2 min(col / floatNumCols, row / floatNumRows);
            AZ::Vector2 max((col + 1) / floatNumCols, (row + 1) / floatNumRows);

            UiTransformInterface::RectPoints uvCellCoords;
            uvCellCoords.TopLeft() = AZ::Vector2(min.GetX(), min.GetY());
            uvCellCoords.BottomLeft() = AZ::Vector2(min.GetX(), max.GetY());
            uvCellCoords.TopRight() = AZ::Vector2(max.GetX(), min.GetY());
            uvCellCoords.BottomRight() = AZ::Vector2(max.GetX(), max.GetY());

            ISprite::SpriteSheetCell newCell;
            newCell.uvCellCoords = uvCellCoords;

            sprite->AddSpriteSheetCell(newCell);
        }
    }

    // Dialog needs to be updated to reflect new sprite-sheet cell info
    ResetUi();
}

void SpriteBorderEditor::DisplaySelectedCell(AZ::u32 cellIndex)
{
    m_currentCellIndex = cellIndex;
    emit SelectedCellChanged(m_sprite, cellIndex);

    // A new cell has been selected, so remove the currently
    // displayed image/cell from the view
    m_cellPropertiesGraphicsScene->removeItem(m_cellPropertiesPixmap);

    // Determine how much we need to scale the view to fit the cell 
    // contents to the displayed properties image.
    const AZ::Vector2 cellSize = m_sprite->GetCellSize(cellIndex);

    // Scale-to-fit, while preserving aspect ratio.
    QRect croppedRect = m_unscaledSpriteSheet.rect();
    {
        const UiTransformInterface::RectPoints& cellUvCoords = m_sprite->GetSourceCellUvCoords(cellIndex);
        int minX = aznumeric_cast<int>(cellUvCoords.TopLeft().GetX() > 0.0f ? croppedRect.right() * cellUvCoords.TopLeft().GetX() : 0);
        int maxX = aznumeric_cast<int>(cellUvCoords.BottomRight().GetX() > 0.0f ? croppedRect.right() * cellUvCoords.BottomRight().GetX() : 0);
        int minY = aznumeric_cast<int>(cellUvCoords.TopLeft().GetY() > 0.0f ? croppedRect.bottom() * cellUvCoords.TopLeft().GetY() : 0);
        int maxY = aznumeric_cast<int>(cellUvCoords.BottomRight().GetY() > 0.0f ? croppedRect.bottom() * cellUvCoords.BottomRight().GetY() : 0);
        croppedRect.setCoords(minX, minY, maxX, maxY);
    }

    // Finally, display the cropped pixmap to show the 
    // selected cell.
    QPixmap croppedPixmap = m_unscaledSpriteSheet.copy(croppedRect).scaled(UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH, UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT, Qt::KeepAspectRatio);
    m_cellPropertiesPixmap = m_cellPropertiesGraphicsScene->addPixmap(croppedPixmap);

    // Render the sprite-sheet cell before the first slice manipulator so the
    // cell doesn't render on top (and occlude) the manipulator.
    m_cellPropertiesPixmap->stackBefore(m_manipulators[0]);

    // Adjust the slice manipulators to the selected cell's border values
    for (int i = 0; i < numSliceBorders; ++i)
    {
        const SpriteBorder border = static_cast<SpriteBorder>(i);
        const float sizeInPixels = IsBorderVertical(border) ? cellSize.GetX() : cellSize.GetY();

        m_manipulators[i]->SetCellIndex(cellIndex);
        m_manipulators[i]->SetPixmapSizes(QSize(aznumeric_cast<int>(cellSize.GetX()), aznumeric_cast<int>(cellSize.GetY())), croppedPixmap.size());
        m_manipulators[i]->setPixelPosition(GetBorderValueInPixels(m_sprite, border, sizeInPixels, cellIndex));
    }

    // Update the texture size text to accurately reflect the new selection
    SetDisplayedTextureSize(cellSize.GetX(), cellSize.GetY());

    // Update cell alias field info
    m_cellAliasLineEdit->setText(m_sprite->GetCellAlias(cellIndex).c_str());
}

void SpriteBorderEditor::AddConfigureSection(QGridLayout* gridLayout, int& rowNum)
{
    QLabel* labelHeader = new QLabel(QString("<h2>Configure Spritesheet</h2>"), this);
    gridLayout->addWidget(labelHeader, rowNum++, 0, 1, 6);

    // Derive row/col based on spritesheet cell UV coord info. Assumes 
    // uniform grid of cells.
    AZStd::unordered_set<float> uSet, vSet;
    for (auto spriteSheetCell : m_sprite->GetSpriteSheetCells())
    {
        uSet.insert(spriteSheetCell.uvCellCoords.TopLeft().GetX());
        uSet.insert(spriteSheetCell.uvCellCoords.TopRight().GetX());

        vSet.insert(spriteSheetCell.uvCellCoords.TopLeft().GetY());
        vSet.insert(spriteSheetCell.uvCellCoords.BottomLeft().GetY());
    }

    // Count the number of unique entries along each axis to determine number
    // of rows/cols contained within the spritesheet.
    m_numRows = static_cast<uint>(vSet.size() > 1 ? vSet.size() - 1 : 1);
    m_numCols = static_cast<uint>(uSet.size() > 1 ? uSet.size() - 1 : 1);

    // Text input fields displaying row/col information for auto-extracting 
    // spritesheet cells
    QLineEdit* numRowsLineEdit = new QLineEdit(QString::number(m_numRows), this);
    QLineEdit* numColsLineEdit = new QLineEdit(QString::number(m_numCols), this);

    numRowsLineEdit->setFixedWidth(textInputWidth);
    numColsLineEdit->setFixedWidth(textInputWidth);

    // Once the user enters in the new row/col information, this callback
    // will notify the SpriteBorderEditor so that the UV information can
    // be auto-generated for each of the cells.
    AZStd::function<void()> rowColChangedCallback =
        [this, numRowsLineEdit, numColsLineEdit]()
        {
            bool rowConversionSuccess = false;
            int newNumRows = numRowsLineEdit->text().toInt(&rowConversionSuccess);

            bool colConversionSuccess = false;
            int newNumCols = numColsLineEdit->text().toInt(&colConversionSuccess);

            const bool positiveInputs = newNumRows > 0 && newNumCols > 0;
            const bool valueChanged = m_numRows != static_cast<uint>(newNumRows) || m_numCols != static_cast<uint>(newNumCols);

            // This number of cells is just nearly unusable in the sprite editor UI. Supporting
            // more would likely require reworking of UX/UI and even implementation.
            static const int maxNumCellsSupported = 32 * 32;

            const bool numCellsSupported = newNumRows * newNumCols <= maxNumCellsSupported;
            const bool inputValid = rowConversionSuccess && colConversionSuccess && positiveInputs && valueChanged && numCellsSupported;
            if (inputValid)
            {
                UpdateSpriteSheetCellInfo(newNumRows, newNumCols, m_sprite);
            }
            else 
            {
                // Restore the current values
                numRowsLineEdit->setText(QString::number(m_numRows));
                numColsLineEdit->setText(QString::number(m_numCols));

                if (!numCellsSupported)
                {
                    QString warningMessage = QString(
                        "Too many rows and columns have been specified!\n"
                        "The maximum number of sprite-sheet cells is limited to %1").arg(maxNumCellsSupported);
                    QMessageBox(QMessageBox::Warning,
                        "Warning",
                        warningMessage,
                        QMessageBox::Ok, QApplication::activeWindow()).exec();
                }
            }
        };

    // Hook up the callback to the text input fields
    QObject::connect(numRowsLineEdit, &QLineEdit::editingFinished, this, rowColChangedCallback);
    QObject::connect(numColsLineEdit, &QLineEdit::editingFinished, this,rowColChangedCallback);

    // Create a new "inner layout" for this section of the window so we can 
    // properly align the content of this section with the other sections by
    // setting margins for the content. This could also possibly be achieved
    // via QSpacerItems.
    QGridLayout* innerLayout = new QGridLayout();
    gridLayout->addLayout(innerLayout, rowNum++, 0, 1, 6, Qt::AlignLeft);

    // These margins effectively indent the content of this entire section to
    // align with the rest of the window contents.
    int left, top, right, bottom;
    innerLayout->getContentsMargins(&left, &top, &right, &bottom);
    innerLayout->setContentsMargins(
        sectionContentLeftMargin, 
        sectionContentTopMargin, 
        right, 
        sectionContentBottomMargin);

    // Finally, add the widgets to the layout
    int innerLayoutCol = 0;
    innerLayout->addWidget(new QLabel("Rows", this),                0, innerLayoutCol++, Qt::AlignLeft);
    innerLayout->addItem(new QSpacerItem(interElementSpacing, 0),   0, innerLayoutCol++, Qt::AlignLeft);
    innerLayout->addWidget(numRowsLineEdit,                         0, innerLayoutCol++, Qt::AlignLeft);
    innerLayout->addItem(new QSpacerItem(interElementSpacing, 0),   0, innerLayoutCol++, Qt::AlignLeft);
    innerLayout->addWidget(new QLabel("Columns", this),             0, innerLayoutCol++, Qt::AlignLeft);
    innerLayout->addItem(new QSpacerItem(interElementSpacing, 0),   0, innerLayoutCol++, Qt::AlignLeft);
    innerLayout->addWidget(numColsLineEdit,                         0, innerLayoutCol++, Qt::AlignLeft);

    // Configure tab order for fields
    QWidget::setTabOrder(numRowsLineEdit, numColsLineEdit);

    // Prime for transition to next "tab-able" field 
    m_prevTabField = numColsLineEdit;
}

void SpriteBorderEditor::AddSelectCellSection(QGridLayout* gridLayout, int& rowNum)
{
    static const int cellSelectionLabelRowSpan = 1;
    static const int cellSelectionLabelColSpan = 6;
    gridLayout->addWidget(new QLabel(QString("<h2>Select cell</h2>"), this), rowNum++, 0, cellSelectionLabelRowSpan, cellSelectionLabelColSpan);

    // The border margin is used to reserve space along the X and Y axes to
    // insert a border in the graphics scene. This way the image fits within
    // the border rather than behind it.
    static const float borderMargin = 2.0f;

    // Total amount of space the border margin occupies along a single axis
    // (which is just double the border margin since the border appears on
    // all edges of the image).
    static const float borderMarginTotal = borderMargin * 2.0f;

    // If the sprite-sheet has a width at least twice as big as its height,
    // then display the image at a bigger size to fill up the contents of
    // the dialog in a more visually appealing way.
    int widthMultiplier = 1;

    // Load the full spritesheet image and scale it to fit the view
    QPixmap scaledPixmap;
    {
        QString fullPath =
            Path::GamePathToFullPath((m_sprite->GetTexturePathname() + "." + AZ::RPI::StreamingImageAsset::Extension).c_str());
        m_unscaledSpriteSheet = QPixmap(fullPath);
        if ((m_unscaledSpriteSheet.height() <= 0) &&
            (m_unscaledSpriteSheet.width() <= 0))
        {
            m_hasBeenInitializedProperly = false;
            return;
        }

        float widthToHeightRatio = static_cast<float>(m_unscaledSpriteSheet.width()) / m_unscaledSpriteSheet.height();
        bool isVertical = (m_unscaledSpriteSheet.height() > m_unscaledSpriteSheet.width());

        if (isVertical)
        {
            scaledPixmap = m_unscaledSpriteSheet.scaledToHeight(aznumeric_cast<int>(UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT - (borderMarginTotal)));
        }
        else
        {
            widthMultiplier = widthToHeightRatio >= 2.0f ? 2 : 1;
            scaledPixmap = m_unscaledSpriteSheet.scaledToWidth(aznumeric_cast<int>(UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH * widthMultiplier - (borderMarginTotal)));
        }
    }

    // Create the graphics scene area to include enough space for the scaled
    // pixmap and the border.
    QGraphicsScene* graphicsScene = new QGraphicsScene(0.0f, 0.0f, scaledPixmap.width() + borderMarginTotal, scaledPixmap.height() + borderMarginTotal, this);

    // Offset the pixmap to fit within the border margins
    QGraphicsPixmapItem* pixmapItem = graphicsScene->addPixmap(scaledPixmap);
    const QPointF scaledPixmapOffset(borderMargin, borderMargin);
    pixmapItem->setOffset(scaledPixmapOffset);

    // Create an "inner layout" to set margins on the pixmap to align with 
    // the rest of the contents of the dialog
    QGridLayout* innerLayout = new QGridLayout();
    {
        gridLayout->addLayout(innerLayout, rowNum, 0, 1, 6, Qt::AlignLeft);

        // Set content margins
        int left, top, right, bottom;
        innerLayout->getContentsMargins(&left, &top, &right, &bottom);
        innerLayout->setContentsMargins(
            sectionContentLeftMargin,
            sectionContentTopMargin,
            right,
            sectionContentBottomMargin);
    }

    static const int cellSelectionRowSpan = 1;
    static const int cellSelectionColSpan = 4;
    QGraphicsView* slicerView = new SlicerView(graphicsScene, this);
    innerLayout->addWidget(slicerView, 0, 0, cellSelectionRowSpan, cellSelectionColSpan, Qt::AlignHCenter | Qt::AlignVCenter);

    // Multiplier to scale individual cells by to fit selection view
    AZ::Vector2 scaleMultiplierWithoutBorder(
        static_cast<float>(scaledPixmap.width() + borderMarginTotal) / m_unscaledSpriteSheet.width(),
        static_cast<float>(scaledPixmap.height() + borderMarginTotal) / m_unscaledSpriteSheet.height());

    // Size of an individual cell after being scaled to fit selection view
    AZ::Vector2 cellSize(
        (m_unscaledSpriteSheet.width() / m_numCols) * scaleMultiplierWithoutBorder.GetX(),
        (m_unscaledSpriteSheet.height() / m_numRows) * scaleMultiplierWithoutBorder.GetY());

    // Add grid overlay on-top of spritesheet image to help visualize 
    // row/col grid of sprite-sheet cells.
    {
        QPen cellDividerPenWhite;
        cellDividerPenWhite.setStyle(Qt::DashLine);
        static const float dashedPenWidth = 2.0f;
        cellDividerPenWhite.setWidthF(dashedPenWidth);
        cellDividerPenWhite.setColor(Qt::white);

        QPen cellDividerPenBlack(cellDividerPenWhite);
        cellDividerPenBlack.setColor(Qt::black);
        cellDividerPenBlack.setStyle(Qt::SolidLine);
        cellDividerPenBlack.setWidthF(dashedPenWidth * 2.0f);

        for (uint row = 0; row < m_numRows; ++row)
        {
            const float yOffset = row * cellSize.GetY();

            // Only add the dashed border to the bottom of this row if we're
            // not on the final/bottom row of the spritehset. The outer sprite-sheet
            // image already has a border.
            const bool finalRow = row == m_numRows - 1;
            if (!finalRow)
            {
                const float bottomOfCellOffset = cellSize.GetY();

                QLineF bottomDashedRowBorderBlack(
                    0.0f,
                    yOffset + bottomOfCellOffset,
                    scaledPixmap.width() + borderMarginTotal,
                    yOffset + bottomOfCellOffset);
                graphicsScene->addLine(bottomDashedRowBorderBlack, cellDividerPenBlack);

                QLineF bottomDashedRowBorderWhite(
                    0.0f,
                    yOffset + bottomOfCellOffset,
                    scaledPixmap.width() + borderMarginTotal,
                    yOffset + bottomOfCellOffset);
                graphicsScene->addLine(bottomDashedRowBorderWhite, cellDividerPenWhite);
            }
        }

        for (uint col = 1; col < m_numCols; ++col)
        {
            const float xOffset = col * cellSize.GetX();

            // Only add the dashed border to the right of the cell if we're
            // not on the last column of the row. The outer sprite-sheet 
            // image already has a border.
            QLineF rightDashedCellBorderBlack(
                xOffset,
                0.0f,
                xOffset,
                scaledPixmap.height() + borderMarginTotal);
            graphicsScene->addLine(rightDashedCellBorderBlack, cellDividerPenBlack);

            QLineF rightDashedCellBorderWhite(
                xOffset,
                0.0f,
                xOffset,
                scaledPixmap.height() + borderMarginTotal);
            graphicsScene->addLine(rightDashedCellBorderWhite, cellDividerPenWhite);
        }
    }

    // Add image border to the scene
    {
        static const float outerPenWidth = borderMargin;
        static const float innerPenWidth = outerPenWidth * 0.5f;

        AZ::Vector2 scaleMultiplierWithBorder(
            static_cast<float>(scaledPixmap.width()) / m_unscaledSpriteSheet.width(),
            static_cast<float>(scaledPixmap.height()) / m_unscaledSpriteSheet.height());

        // Outer, black border
        {
            QPen wholeImageBorder;
            wholeImageBorder.setWidthF(outerPenWidth);
            wholeImageBorder.setColor(Qt::black);
            wholeImageBorder.setJoinStyle(Qt::MiterJoin);

            QPointF topLeft(outerPenWidth * 0.5f, outerPenWidth * 0.5f);
            QPointF bottomRight(
                outerPenWidth + m_unscaledSpriteSheet.width() * scaleMultiplierWithBorder.GetX() + 1.0f,
                outerPenWidth + m_unscaledSpriteSheet.height() * scaleMultiplierWithBorder.GetY() + 1.0f);
            QRectF cellRect(topLeft, bottomRight);

            graphicsScene->addRect(cellRect, wholeImageBorder);
        }

        // Inner, white border
        {
            QPen wholeImageBorder;
            wholeImageBorder.setWidthF(innerPenWidth);
            wholeImageBorder.setColor(Qt::white);
            wholeImageBorder.setJoinStyle(Qt::MiterJoin);

            QPointF topLeft(outerPenWidth, outerPenWidth);
            QPointF bottomRight(
                topLeft.x() + m_unscaledSpriteSheet.width() * scaleMultiplierWithBorder.GetX() - outerPenWidth + 1.0f,
                topLeft.y() + m_unscaledSpriteSheet.height() * scaleMultiplierWithBorder.GetY() - outerPenWidth + 1.0f);
            QRectF cellRect(topLeft, bottomRight);

            graphicsScene->addRect(cellRect, wholeImageBorder);
        }
    }

    // Finally, add invisible rect items to the scene that correspond to each
    // cell of the sprite-sheet. Each rect item has a callback that processes
    // which cell of the sprite-sheet was selected.
    for (uint row = 0; row < m_numRows; ++row)
    {
        const float yOffset = row * cellSize.GetY();

        for (uint col = 0; col < m_numCols; ++col)
        {
            const bool topRow = row == 0;
            const bool firstColumnInRow = col == 0;
            const bool lastColumnInRow = col == m_numCols - 1;
            const bool bottomRow = row == m_numRows - 1;

            const float xOffset = col * cellSize.GetX();
            const float borderMarginRectOffset = borderMargin * 0.5f + 1.0f;
            const float topLeftXOffset = firstColumnInRow ? borderMarginRectOffset : 0.0f;
            const float topLeftYOffset = topRow ? borderMarginRectOffset : 0.0f;
            const float bottomRightYOffset = bottomRow ? borderMarginRectOffset : 0.0f;

            // The right border of the cell selection rect gets clipped (due
            // to the way the QPen renders) when the last column cell in the
            // row is selected.
            const float lastColOffset = lastColumnInRow ? 2.0f : 0.0f;

            // Calculate the top-left and bottom-right coordinates for this
            // cell within the cell selection graphics view.
            QPointF topLeft(xOffset + topLeftXOffset, yOffset + topLeftYOffset);
            QPointF bottomRight(xOffset + cellSize.GetX() - lastColOffset, yOffset + cellSize.GetY() - bottomRightYOffset);

            // Create the graphics rect item with a custom mouse press event
            // that allows us to get information of the selected cell.
            QRectF cellRect(topLeft, bottomRight);
            int cellIndex = row * m_numCols + col;
            CellSelectRectItem* cellSelectRectItem = new CellSelectRectItem(
                cellRect,
                [this, cellIndex]()
            {
                DisplaySelectedCell(cellIndex);
            });

            QObject::connect(
                this,
                &SpriteBorderEditor::ResettingUi,
                cellSelectRectItem,
                &CellSelectRectItem::StopProcessingInput);

            cellSelectRectItem->setPen(Qt::NoPen);

            graphicsScene->addItem(cellSelectRectItem);

            // Pre-select the first cell
            const bool firstCell = row == 0 && col == 0;
            if (firstCell)
            {
                cellSelectRectItem->SelectCell();
            }
        }
    }

    rowNum += cellSelectionRowSpan;
}

void SpriteBorderEditor::AddPropertiesSection(QGridLayout* gridLayout, int& rowNum)
{
    gridLayout->addWidget(new QLabel(QString("<h2>Border Properties</h2>"), this), rowNum++, 0, 1, 6);

    // Create an "inner layout" to set margins on the pixmap to align with 
    // the rest of the contents of the dialog.
    QGridLayout* innerLayout = new QGridLayout();
    {
        gridLayout->addLayout(innerLayout, rowNum, 0, 6, 8);

        // Set content margins
        int left, top, right, bottom;
        innerLayout->getContentsMargins(&left, &top, &right, &bottom);
        innerLayout->setContentsMargins(
            sectionContentLeftMargin,
            sectionContentTopMargin,
            right,
            sectionContentBottomMargin);
    }

    // The scene and view responsible for displaying the image (or image of
    // a specific spritesheet cell).
    m_cellPropertiesGraphicsScene = new QGraphicsScene(0.0f, 0.0f, UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH, UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT, this);
    innerLayout->addWidget(new SlicerView(m_cellPropertiesGraphicsScene, this), 0, 0, 1, 1, Qt::AlignLeft);

    // The image (or spritesheet cell).
    QSize unscaledPixmapSize;
    QSize scaledPixmapSize;
    {
        QString fullPath =
            Path::GamePathToFullPath((m_sprite->GetTexturePathname() + "." + AZ::RPI::StreamingImageAsset::Extension).c_str());
        m_unscaledSpriteSheet = QPixmap(fullPath);
        if ((m_unscaledSpriteSheet.size().height() <= 0) &&
            (m_unscaledSpriteSheet.size().width() <= 0))
        {
            m_hasBeenInitializedProperly = false;
            return;
        }

        bool isVertical = (m_unscaledSpriteSheet.size().height() > m_unscaledSpriteSheet.size().width());

        // Scale-to-fit, while preserving aspect ratio.
        QPixmap scaledPixmap;

        if (isVertical)
        {
            scaledPixmap = m_unscaledSpriteSheet.scaledToHeight(UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT);
        }
        else
        {
            scaledPixmap = m_unscaledSpriteSheet.scaledToWidth(UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH);
        }

        m_cellPropertiesPixmap = m_cellPropertiesGraphicsScene->addPixmap(scaledPixmap);

        unscaledPixmapSize = m_unscaledSpriteSheet.size();
        scaledPixmapSize = m_cellPropertiesPixmap->pixmap().size();
    }

    // The properties section is divided into two columns. The left column
    // displays the currently selected cell, and this right column displays
    // the modifiable border properties. A separate grid layout is created to
    // achieve desired visual layout.
    QGridLayout* rightColumnLayout= new QGridLayout();
    {
        innerLayout->addLayout(rightColumnLayout, 0, 1, 1, 1);

        // Separate layout for formatting texture/cell size label
        QGridLayout* textureSizeLabelLayout = new QGridLayout();
        {
            rightColumnLayout->addLayout(textureSizeLabelLayout, 0, 0, 1, 3, Qt::AlignTop);

            int left, top, right, bottom;
            textureSizeLabelLayout->getContentsMargins(&left, &top, &right, &bottom);
            textureSizeLabelLayout->setContentsMargins(
                interElementSpacing,
                top,
                right,
                bottom);

            m_textureSizeLabel = new QLabel(this);
            textureSizeLabelLayout->addWidget(m_textureSizeLabel, 0, 0, Qt::AlignLeft);
            SetDisplayedTextureSize(aznumeric_cast<float>(unscaledPixmapSize.width()), aznumeric_cast<float>(unscaledPixmapSize.height()));
        }

        // Separate layout for border property fields
        QGridLayout* propertyFieldsLayout = new QGridLayout();
        {
            rightColumnLayout->addLayout(propertyFieldsLayout, 1, 0, 8, 3, Qt::AlignTop | Qt::AlignLeft);

            int left, top, right, bottom;
            propertyFieldsLayout->getContentsMargins(&left, &top, &right, &bottom);
            propertyFieldsLayout->setContentsMargins(
                interElementSpacing,
                top,
                right,
                bottom);

            // Row value/iterator for row placement within layout
            int row = 0;

            // Text field for modifying cell string alias
            int columnCount = 0;
            propertyFieldsLayout->addWidget(new QLabel("Alias", this), row, columnCount++, Qt::AlignLeft);
            propertyFieldsLayout->addItem(new QSpacerItem(interElementSpacing, 0), row, columnCount++, Qt::AlignLeft);

            m_cellAliasLineEdit = new QLineEdit(this);
            m_cellAliasLineEdit->setFixedWidth(textInputWidth);
            propertyFieldsLayout->addWidget(m_cellAliasLineEdit, row, columnCount++, Qt::AlignLeft);

            // Editing finished callback for setting alias value after being entered
            QObject::connect(m_cellAliasLineEdit, &QLineEdit::editingFinished, this,
                [this]()
                {
                    // Remove preceding or trailing whitespace and tab/newline chars
                    const QString lineEditText = m_cellAliasLineEdit->text().simplified();

                    // Only allow alphanumeric and whitespace chars
                    QRegExp re("([A-Z]|[a-z]|[0-9]|\\s)*");
                    const bool containsOnlyAlphaNumeric = re.exactMatch(lineEditText);
                    const bool hasValidLength = lineEditText.length() <= 128;
                    const bool lineEditTextValid = containsOnlyAlphaNumeric && hasValidLength;
                    if (lineEditTextValid)
                    {
                        m_sprite->SetCellAlias(m_currentCellIndex, lineEditText.toUtf8().constData());

                        const bool wasSimplified = lineEditText != m_cellAliasLineEdit->text();
                        if (wasSimplified)
                        {
                            // Update line edit text to simplified value
                            m_cellAliasLineEdit->setText(lineEditText);

                            // Tell the user that the value was simplified, but not in the case where
                            // the string is empty anywas (user accidentally hits space character or
                            // something).
                            if (!lineEditText.isEmpty())
                            {
                                QMessageBox(QMessageBox::Information,
                                    "Alias Value Updated",
                                    "The cell alias that was entered has been modified to remove additional whitespace characters.",
                                    QMessageBox::Ok, QApplication::activeWindow()).exec();
                            }
                        }
                    }
                    else
                    {
                        if (!containsOnlyAlphaNumeric)
                        {
                            QMessageBox(QMessageBox::Warning,
                                "Warning",
                                "Unable to set cell alias value. Only alphanumeric characters are supported.",
                                QMessageBox::Ok, QApplication::activeWindow()).exec();
                        }
                        else
                        {
                            QMessageBox(QMessageBox::Warning,
                                "Warning",
                                "Unable to set cell alias value. The alias is too long.",
                                QMessageBox::Ok, QApplication::activeWindow()).exec();
                        }

                        // Restore original line edit text value
                        m_cellAliasLineEdit->setText(m_sprite->GetCellAlias(m_currentCellIndex).c_str());
                    }
                    
                });

            // Prime row value for the following (border value) fields
            ++row;

            // Used for setting tab order
            SlicerEdit* prevEditField = nullptr;

            for (const auto b : SpriteBorder())
            {
                SlicerEdit* edit = new SlicerEdit(
                    this,
                    b,
                    unscaledPixmapSize,
                    m_sprite);

                SlicerManipulator* manipulator = new SlicerManipulator(b,
                    unscaledPixmapSize,
                    scaledPixmapSize,
                    m_sprite,
                    m_cellPropertiesGraphicsScene,
                    edit);

                int manipulatorArrayIndex = static_cast<int>(b);
                m_manipulators[manipulatorArrayIndex] = manipulator;

                edit->SetManipulator(manipulator);
                edit->setFixedWidth(textInputWidth);

                int innerLayoutCol = 0;
                propertyFieldsLayout->addWidget(new QLabel(SpriteBorderToString(b), this), row, innerLayoutCol++, Qt::AlignLeft);
                propertyFieldsLayout->addItem(new QSpacerItem(interElementSpacing, 0), row, innerLayoutCol++, Qt::AlignLeft);
                propertyFieldsLayout->addWidget(edit, row, innerLayoutCol++, Qt::AlignLeft);
                propertyFieldsLayout->addWidget(new QLabel("px", this), row, innerLayoutCol++, Qt::AlignLeft);
                row++;

                // Setup tab order
                if (prevEditField)
                {
                    QWidget::setTabOrder(prevEditField, edit);
                }
                else
                {
                    // Tab from previous tab-able field to alias field
                    if (m_prevTabField)
                    {
                        QWidget::setTabOrder(m_prevTabField, m_cellAliasLineEdit);
                    }
                    
                    // Need to transition from alias field to first border
                    // edit field since the alias field comes first.
                    QWidget::setTabOrder(m_cellAliasLineEdit, edit);
                }

                prevEditField = edit;
            }
        }
    }
}

void SpriteBorderEditor::AddButtonsSection(QGridLayout* gridLayout, int& rowNum)
{
    // Add a button to allow users to configure this image as a sprite-sheet,
    // otherwise hide it if they already are working with a sprite-sheet.
    if (!IsConfiguringSpriteSheet())
    {
        // Left-align the button
        QGridLayout* leftAlignedLayout = new QGridLayout();
        gridLayout->addLayout(leftAlignedLayout, rowNum, 0, Qt::AlignLeft);

        QPushButton* configureButton = new QPushButton("Configure Spritesheet", this);

        QObject::connect(configureButton,
            &QPushButton::clicked, this,
            [this]([[maybe_unused]] bool checked)
        {
            m_configureAsSpriteSheet = true;
            ResetUi();
        });

        leftAlignedLayout->addWidget(configureButton, rowNum, 0);
    }

    // Needed to right-align buttons next to eachother
    QGridLayout* innerLayout = new QGridLayout();
    gridLayout->addLayout(innerLayout, rowNum, 1, Qt::AlignRight);

    // Add buttons.
    {
        // Save button.
        QPushButton* saveButton = new QPushButton("Save", this);
        QObject::connect(saveButton,
            &QPushButton::clicked, this,
            [this]([[maybe_unused]] bool checked)
            {
                // Sanitize values.
                //
                // This is the simplest way to sanitize the
                // border values. Otherwise, we need to prevent
                // flipping the manipulators in the UI.
                {
                    ISprite::Borders b = m_sprite->GetBorders();

                    if (b.m_top > b.m_bottom)
                    {
                        std::swap(b.m_top, b.m_bottom);
                    }

                    if (b.m_left > b.m_right)
                    {
                        std::swap(b.m_left, b.m_right);
                    }

                    m_sprite->SetBorders(b);
                }

                // the sprite file may not exist yet. If it does not then GamePathToFullPath
                // will give a path in the project even if the texture is in a gem.
                // The texture is guaranteed to exist so use that to get the full path.
                QString fullTexturePath =
                    Path::GamePathToFullPath((m_sprite->GetTexturePathname() + "." + AZ::RPI::StreamingImageAsset::Extension).c_str());
                const char* const spriteExtension = "sprite";
                AZStd::string fullSpritePath = PathUtil::ReplaceExtension(fullTexturePath.toUtf8().data(), spriteExtension);

                FileHelpers::SourceControlAddOrEdit(fullSpritePath.c_str(), QApplication::activeWindow());

                bool saveSuccessful = m_sprite->SaveToXml(fullSpritePath.c_str());

                if (saveSuccessful)
                {
                    accept();
                    return;
                }

                QMessageBox(QMessageBox::Critical,
                    "Error",
                    "Unable to save file. Is the file read-only?",
                    QMessageBox::Ok, QApplication::activeWindow()).exec();
            });
        saveButton->setProperty("class", "Primary");
        innerLayout->addWidget(saveButton, rowNum, 0);

        // Cancel button.
        QPushButton* cancelButton = new QPushButton("Cancel", this);
        QObject::connect(cancelButton,
            &QPushButton::clicked, this,
            [this]([[maybe_unused]] bool checked)
        {
            // Since we're cancelling the dialog, restore the original sprite
            // configuration from when the dialog originally opened.
            m_sprite->SetSpriteSheetCells(m_restoreInfo.spriteSheetCells);
            m_sprite->SetBorders(m_restoreInfo.borders);

            reject();
        });
        innerLayout->addWidget(cancelButton, rowNum, 1);
    }
}

void SpriteBorderEditor::AddSeparator(QGridLayout* gridLayout, int& rowNum)
{
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    const int firstColumnPosition = 0;
    const int singleRowSpan = 1;
    const int fullWindowWidthColumnSpan = 8;
    gridLayout->addWidget(line, rowNum++, firstColumnPosition, singleRowSpan, fullWindowWidthColumnSpan);
}

void SpriteBorderEditor::SetDisplayedTextureSize(float width, float height)
{
    QString imageDescription = m_sprite->GetSpriteSheetCells().size() <= 1 ? "Texture" : "Cell size";
    m_textureSizeLabel->setText(QString("%1 is %2 x %3").arg(imageDescription).arg(QString::number(width)).arg(QString::number(height)));
}

bool SpriteBorderEditor::IsConfiguringSpriteSheet() const
{
    return m_sprite->IsSpriteSheet() || m_configureAsSpriteSheet;
}

CellSelectRectItem::CellSelectRectItem(const QRectF& rect, const AZStd::function<void()>& clickCallback)
    : QGraphicsRectItem(rect)
    , m_clickCallback(clickCallback) 
{ 
}

CellSelectRectItem::~CellSelectRectItem()
{
    // We assume that the layout is being reset/cleared when this
    // dtor is getting called. It's possible that a mousePressEvent
    // has already been invoked on a newer CellSelectRectItem. If that's
    // the case, the current selection ptr will be dangling, so just clear
    // it here.
    ClearSelection();
}

void CellSelectRectItem::SelectCell()
{
    if (s_currentSelection)
    {
        s_currentSelection->setPen(Qt::NoPen);
    }

    s_currentSelection = this;

    QPen solidPenStyle;
    static const QColor orangeQColor(255, 165, 0);
    solidPenStyle.setColor(orangeQColor);
    solidPenStyle.setStyle(Qt::SolidLine);
    solidPenStyle.setWidth(4);
    solidPenStyle.setJoinStyle(Qt::MiterJoin);
    setPen(solidPenStyle);
}

void CellSelectRectItem::mousePressEvent([[maybe_unused]] QGraphicsSceneMouseEvent* mouseEvent)
{
    if (m_processInput)
    {
        SelectCell();
        m_clickCallback();
    }
}

#include <moc_SpriteBorderEditor.cpp>

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiImageBus.h>

//! On-screen control used to modify border info for 9-slicing sprites.
class SlicerManipulator
    : public QGraphicsRectItem
{
public:

    SlicerManipulator(SpriteBorder border,
        QSize& unscaledPixmapSize,
        QSize& scaledPixmapSize,
        ISprite* sprite,
        QGraphicsScene* scene,
        SlicerEdit* edit);

    //! Associates a text input/QLineEdit control with this manipulator.
    void SetEdit(SlicerEdit* edit);

    //! Provides the scale and unscale sprite sizes as displayed in the properties pane.
    //!
    //! This method is intended to be called when the displayed pixmap/image 
    //! changes, such as when the user selects a different cell of a sprite-sheet.
    //!
    //! The scale values themselves are primarily used to convert between viewport 
    //! and spritesheet cell texture spaces.
    void SetPixmapSizes(const QSize& unscaledSize, const QSize& scaledSize);

    //! A cell index can be provided when working with sprite-sheets.
    //!
    //! The cell index is used to set the border info on the sprite-sheet
    //! cell as the border values are manipulated by the user with this
    //! manipulator.
    void SetCellIndex(AZ::u32 cellIndex) { m_cellIndex = cellIndex; };

    //! Changes the on-screen position of this manipulator based on the new border pixel value.
    void setPixelPosition(float p);

protected:

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:

    SpriteBorder m_border;
    bool m_isVertical;
    QSize m_unscaledPixmapSize;
    QSize m_scaledPixmapSize;
    ISprite* m_sprite;
    QPointF m_unscaledOverScaledFactor;
    QPointF m_scaledOverUnscaledFactor;
    QPen m_penFront;
    QPen m_penBack;

    SlicerEdit* m_edit;

    AZ::u32 m_cellIndex = 0;   //!< The cell index currently displayed to the user (if applicable).
};

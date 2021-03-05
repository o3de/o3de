//-------------------------------------------------------------------------------
// Copyright (C) Amazon.com, Inc. or its affiliates.
// All Rights Reserved.
//
// Licensed under the terms set out in the LICENSE.HTML file included at the
// root of the distribution; you may not use this file except in compliance
// with the License.
//
// Do not remove or modify this notice or the LICENSE.HTML file.  This file
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// either express or implied. See the License for the specific language
// governing permissions and limitations under the License.
//-------------------------------------------------------------------------------

// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_H
#define CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_H

class SlicerManipulator
: public QGraphicsRectItem
{
public:

    SlicerManipulator( SpriteBorder border,
                        QSize& unscaledPixmapSize,
                        QSize& scaledPixmapSize,
                        float thicknessInPixels,
                        ISprite* sprite,
                        QGraphicsScene* scene );

    void SetEdit(SlicerEdit* edit);

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
    QColor m_color;

    SlicerEdit* m_edit;
};

#endif // CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_H

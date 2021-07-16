/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SpriteBorderEditorCommon.h"

#define UICANVASEDITOR_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR    (0)
#define UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER                     (10000.0f)
#define UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS    (24)
#define UICANVASEDITOR_SLICERMANIPULATOR_DRAW_HALF_WIDTH            (1.0f)

SlicerManipulator::SlicerManipulator(SpriteBorder border,
    QSize& unscaledPixmapSize,
    QSize& scaledPixmapSize,
    ISprite* sprite,
    QGraphicsScene* scene,
    SlicerEdit* edit)
    : QGraphicsRectItem()
    , m_border(border)
    , m_isVertical(IsBorderVertical(m_border))
    , m_unscaledPixmapSize(unscaledPixmapSize)
    , m_scaledPixmapSize(scaledPixmapSize)
    , m_sprite(sprite)
    , m_unscaledOverScaledFactor(((float)m_unscaledPixmapSize.width() / (float)m_scaledPixmapSize.width()),
        ((float)m_unscaledPixmapSize.height() / (float)m_scaledPixmapSize.height()))
    , m_scaledOverUnscaledFactor((1.0f / m_unscaledOverScaledFactor.x()),
        (1.0f / m_unscaledOverScaledFactor.y()))
    , m_penFront(Qt::DotLine)
    , m_penBack()
    , m_edit(edit)
{
    setAcceptHoverEvents(true);

    scene->addItem(this);

    setRect((m_isVertical ? -(UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS * 0.5f) : -UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER),
        (m_isVertical ? -UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER : -(UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS * 0.5f)),
        (m_isVertical ? UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS : (3.0f * UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER)),
        (m_isVertical ? (3.0f * UICANVASEDITOR_ARBITRARILY_LARGE_NUMBER) : UICANVASEDITOR_MANIPULATOR_GRASPABLE_THICKNESS_IN_PIXELS));

    setPixelPosition(GetBorderValueInPixels(m_sprite, m_border, aznumeric_cast<float>(m_isVertical ? m_unscaledPixmapSize.width() : m_unscaledPixmapSize.height())));

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);   // This allows using the CTRL key to select multiple manipulators and move them simultaneously.
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

    m_penFront.setColor(Qt::white);
    m_penBack.setColor(Qt::black);

    m_penFront.setWidthF(2.0f * UICANVASEDITOR_SLICERMANIPULATOR_DRAW_HALF_WIDTH);
    m_penBack.setWidthF(2.0f * UICANVASEDITOR_SLICERMANIPULATOR_DRAW_HALF_WIDTH);
}

void SlicerManipulator::SetEdit(SlicerEdit* edit)
{
    m_edit = edit;
}

void SlicerManipulator::SetPixmapSizes(const QSize& unscaledSize, const QSize& scaledSize)
{
    const bool validInputs = unscaledSize.isValid() && scaledSize.isValid();
    if (!validInputs)
    {
        return;
    }

    m_unscaledPixmapSize = unscaledSize;
    m_scaledPixmapSize = scaledSize;

    m_unscaledOverScaledFactor = QPointF(((float)m_unscaledPixmapSize.width() / (float)m_scaledPixmapSize.width()),
        ((float)m_unscaledPixmapSize.height() / (float)m_scaledPixmapSize.height()));
    m_scaledOverUnscaledFactor = QPointF((1.0f / m_unscaledOverScaledFactor.x()),
        (1.0f / m_unscaledOverScaledFactor.y()));
}

void SlicerManipulator::paint(QPainter* painter, [[maybe_unused]] const QStyleOptionGraphicsItem* option, [[maybe_unused]] QWidget* widget)
{
#if UICANVASEDITOR_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR
    QGraphicsRectItem::paint(painter, option, widget);
#endif // UICANVASEDITOR_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR

    // Draw a thin line in the middle of the selectable area.
    if (m_isVertical)
    {
        float x = aznumeric_cast<float>((rect().left() + rect().right()) * 0.5f);
        float y_start = 0;
        float y_end = aznumeric_cast<float>(m_scaledPixmapSize.height());

        // Back.
        painter->setPen(m_penBack);
        painter->drawLine(aznumeric_cast<int>(x), aznumeric_cast<int>(y_start), aznumeric_cast<int>(x), aznumeric_cast<int>(y_end));

        // Front.
        painter->setPen(m_penFront);
        painter->drawLine(aznumeric_cast<int>(x), aznumeric_cast<int>(y_start), aznumeric_cast<int>(x), aznumeric_cast<int>(y_end));
    }
    else // Horizontal.
    {
        float x_start = 0.0f;
        float x_end = aznumeric_cast<float>(m_scaledPixmapSize.width());
        float y = aznumeric_cast<float>((rect().top() + rect().bottom()) * 0.5f);

        // Back.
        painter->setPen(m_penBack);
        painter->drawLine(aznumeric_cast<int>(x_start), aznumeric_cast<int>(y), aznumeric_cast<int>(x_end), aznumeric_cast<int>(y));

        // Front.
        painter->setPen(m_penFront);
        painter->drawLine(aznumeric_cast<int>(x_start), aznumeric_cast<int>(y), aznumeric_cast<int>(x_end), aznumeric_cast<int>(y));
    }
}

void SlicerManipulator::setPixelPosition(float p)
{
    float xPos = aznumeric_cast<float>(m_isVertical ? (p * m_scaledOverUnscaledFactor.x()) : 0.0f);
    float yPos = aznumeric_cast<float>(m_isVertical ? 0.0f : (p * m_scaledOverUnscaledFactor.y()));

    setPos(xPos, yPos);
}

QVariant SlicerManipulator::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if ((change == ItemPositionChange) &&
        scene())
    {
        const float totalScaledSizeInPixels = aznumeric_cast<float>(m_isVertical ? m_scaledPixmapSize.width() : m_scaledPixmapSize.height());
        float manipulatorPos = aznumeric_cast<float>(AZ::GetMax(m_isVertical ? value.toPointF().x() : value.toPointF().y(), 0.0));
        const float borderValue = aznumeric_cast<float>(m_isVertical ? (manipulatorPos * m_unscaledOverScaledFactor.x()) : (manipulatorPos * m_unscaledOverScaledFactor.y()));
        m_edit->setPixelPosition(borderValue);

        const float cellSize = (m_isVertical ? m_sprite->GetCellSize(m_cellIndex).GetX() : m_sprite->GetCellSize(m_cellIndex).GetY());
        SetBorderValue(m_sprite, m_border, borderValue, cellSize, m_cellIndex);

        manipulatorPos = AZ::GetMin<float>(manipulatorPos, totalScaledSizeInPixels);
        return QPointF((m_isVertical ? manipulatorPos : 0.0f),
            (m_isVertical ? 0.0f : manipulatorPos));
    }

    return QGraphicsItem::itemChange(change, value);
}

void SlicerManipulator::hoverEnterEvent([[maybe_unused]] QGraphicsSceneHoverEvent* event)
{
    setCursor(m_isVertical ? Qt::SizeHorCursor : Qt::SizeVerCursor);
    m_penFront.setColor(Qt::yellow);
    update();
}

void SlicerManipulator::hoverLeaveEvent([[maybe_unused]] QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    m_penFront.setColor(Qt::white);
    update();
}

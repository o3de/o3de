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

#include "EditorCommon_precompiled.h"
#include "SpriteBorderEditorCommon.h"

#define CRYINCLUDE_EDITORCOMMON_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR    ( 0 )
#define CRYINCLUDE_EDITORCOMMON_ARBITRARILY_LARGE_NUMBER                     ( 10000.0f )
#define CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_WIDTH                      ( 2.0f )

SlicerManipulator::SlicerManipulator( SpriteBorder border,
                                        QSize& unscaledPixmapSize,
                                        QSize& scaledPixmapSize,
                                        float thicknessInPixels,
                                        ISprite* sprite,
                                        QGraphicsScene* scene )
: QGraphicsRectItem()
, m_border( border )
, m_isVertical( IsBorderVertical( m_border ) )
, m_unscaledPixmapSize( unscaledPixmapSize )
, m_scaledPixmapSize( scaledPixmapSize )
, m_sprite( sprite )
, m_unscaledOverScaledFactor( ( (float)m_unscaledPixmapSize.width() / (float)m_scaledPixmapSize.width() ),
                                ( (float)m_unscaledPixmapSize.height() / (float)m_scaledPixmapSize.height() ) )
, m_scaledOverUnscaledFactor( ( 1.0f / m_unscaledOverScaledFactor.x() ),
                                ( 1.0f / m_unscaledOverScaledFactor.y() ) )
, m_color( Qt::white )
, m_edit( nullptr )
{
    setAcceptHoverEvents( true );

    scene->addItem( this );

    setRect( ( m_isVertical ? - ( thicknessInPixels * 0.5f ) : - CRYINCLUDE_EDITORCOMMON_ARBITRARILY_LARGE_NUMBER ),
                ( m_isVertical ? - CRYINCLUDE_EDITORCOMMON_ARBITRARILY_LARGE_NUMBER : - ( thicknessInPixels * 0.5f ) ),
                ( m_isVertical ? thicknessInPixels : ( 3.0f * CRYINCLUDE_EDITORCOMMON_ARBITRARILY_LARGE_NUMBER ) ),
                ( m_isVertical ? ( 3.0f * CRYINCLUDE_EDITORCOMMON_ARBITRARILY_LARGE_NUMBER ) : thicknessInPixels ) );

    setPixelPosition( GetBorderValueInPixels( m_sprite, m_border, aznumeric_cast<float>( m_isVertical ? m_unscaledPixmapSize.width() : m_unscaledPixmapSize.height() ) ) );

    setFlag( QGraphicsItem::ItemIsMovable, true );
    setFlag( QGraphicsItem::ItemIsSelectable, true ); // This allows using the CTRL key to select multiple manipulators and move them simultaneously.
    setFlag( QGraphicsItem::ItemSendsScenePositionChanges, true );
}

void SlicerManipulator::SetEdit( SlicerEdit *edit )
{
    m_edit = edit;
}

void SlicerManipulator::paint(QPainter* painter, [[maybe_unused]] const QStyleOptionGraphicsItem* option, [[maybe_unused]] QWidget* widget)
{
#if CRYINCLUDE_EDITORCOMMON_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR
    QGraphicsRectItem::paint( painter, option, widget );
#endif // CRYINCLUDE_EDITORCOMMON_DRAW_SELECTABLE_AREA_OF_SLICERMANIPULATOR

    QPen pen;
    pen.setStyle( isSelected() ? Qt::DashLine : Qt::DotLine );
    pen.setWidthF( CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_WIDTH );

    // Draw a thin line in the middle of the selectable area.
    if( m_isVertical )
    {
        float x = aznumeric_cast<float>( ( ( rect().left() + rect().right() ) * 0.5f ) - CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_WIDTH );

        pen.setColor( m_color );
        painter->setPen( pen );
        painter->drawLine(aznumeric_cast<int>(x), aznumeric_cast<int>(rect().top()), aznumeric_cast<int>(x), aznumeric_cast<int>(rect().bottom()) );

        x += CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_WIDTH;
        pen.setColor( Qt::black );
        painter->setPen( pen );
        painter->drawLine(aznumeric_cast<int>(x), aznumeric_cast<int>(rect().top()), aznumeric_cast<int>(x), aznumeric_cast<int>(rect().bottom()) );
    }
    else
    {
        float y = aznumeric_cast<float>( ( ( rect().top() + rect().bottom() ) * 0.5f ) - CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_WIDTH );

        pen.setColor( m_color );
        painter->setPen( pen );
        painter->drawLine(aznumeric_cast<int>(rect().left()), aznumeric_cast<int>(y), aznumeric_cast<int>(rect().right()), aznumeric_cast<int>(y) );

        y += CRYINCLUDE_EDITORCOMMON_SLICERMANIPULATOR_WIDTH;
        pen.setColor( Qt::black );
        painter->setPen( pen );
        painter->drawLine(aznumeric_cast<int>(rect().left()), aznumeric_cast<int>(y), aznumeric_cast<int>(rect().right()), aznumeric_cast<int>(y) );
    }
}

void SlicerManipulator::setPixelPosition(float p)
{
    setPos( ( m_isVertical ? ( p * m_scaledOverUnscaledFactor.x() ) : 0.0f ),
            ( m_isVertical ? 0.0f : ( p * m_scaledOverUnscaledFactor.y() ) ) );
}

QVariant SlicerManipulator::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if( ( change == ItemPositionChange ) &&
        scene() )
    {
        float totalScaledSizeInPixels = aznumeric_cast<float>( m_isVertical ? m_scaledPixmapSize.width() : m_scaledPixmapSize.height() );

        float p = clamp_tpl<float>(aznumeric_cast<float>( m_isVertical ? value.toPointF().x() : value.toPointF().y() ),
                                    0.0f,
                                    totalScaledSizeInPixels );

        m_edit->setPixelPosition( m_isVertical ? aznumeric_cast<float>( p * m_unscaledOverScaledFactor.x() ) : aznumeric_cast<float>( p * m_unscaledOverScaledFactor.y() ) );

        SetBorderValue( m_sprite, m_border, p, totalScaledSizeInPixels );

        return QPointF( ( m_isVertical ? p : 0.0f ),
                        ( m_isVertical ? 0.0f : p ) );
    }

    return QGraphicsItem::itemChange( change, value );
}

void SlicerManipulator::hoverEnterEvent([[maybe_unused]] QGraphicsSceneHoverEvent* event)
{
    setCursor( m_isVertical ? Qt::SizeHorCursor : Qt::SizeVerCursor );
    m_color = Qt::yellow;
    update();
}

void SlicerManipulator::hoverLeaveEvent([[maybe_unused]] QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    m_color = Qt::white;
    update();
}

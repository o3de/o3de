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

SlicerEdit::SlicerEdit( SpriteBorder border,
                        QSize& unscaledPixmapSize,
                        ISprite* sprite )
: QLineEdit()
, m_manipulator( nullptr )
{
    bool isVertical = IsBorderVertical( border );

    float totalUnscaledSizeInPixels = aznumeric_cast<float>( isVertical ? unscaledPixmapSize.width() : unscaledPixmapSize.height() );

    setPixelPosition( GetBorderValueInPixels( sprite, border, totalUnscaledSizeInPixels ) );

    setValidator( new QDoubleValidator( 0.0f, totalUnscaledSizeInPixels, 1 ) );

    QObject::connect( this,
                        &SlicerEdit::editingFinished, this,
                        [ this, border, sprite, totalUnscaledSizeInPixels ]()
                        {
                            float p = text().toFloat();

                            m_manipulator->setPixelPosition( p );

                            SetBorderValue( sprite, border, p, totalUnscaledSizeInPixels );
                        } );
}

void SlicerEdit::SetManipulator(SlicerManipulator* manipulator)
{
    m_manipulator = manipulator;
}

void SlicerEdit::setPixelPosition(float p)
{
    setText( QString::number( p ) );
}

#include <QPropertyTree/moc_SlicerEdit.cpp>

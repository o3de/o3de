/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "UiCanvasEditor_precompiled.h"
#include "SpriteBorderEditorCommon.h"

SlicerEdit::SlicerEdit(
    SpriteBorderEditor* borderEditor,
    SpriteBorder border,
    [[maybe_unused]] QSize& unscaledPixmapSize,
    ISprite* sprite)
    : QLineEdit()
    , m_sprite(sprite)
    , m_border(border)
{
    bool isVertical = IsBorderVertical(m_border);

    const AZ::Vector2 cellSize = m_sprite->GetCellSize(m_currentCellIndex);
    const float totalUnscaledSizeInPixels = (isVertical ? cellSize.GetX() : cellSize.GetY());

    setPixelPosition(GetBorderValueInPixels(m_sprite, m_border, totalUnscaledSizeInPixels));

    setValidator(new QDoubleValidator(0.0f, totalUnscaledSizeInPixels, 1));

    QObject::connect(this,
        &SlicerEdit::editingFinished,
        this,
        [ this, totalUnscaledSizeInPixels ]()
        {
            // User text input is always interpreted as relative value
            const float relativeBorderValue = text().toFloat();
            
            // Whereas the on-screen manipulator position and stored values
            // are absolute.
            const float absoluteBorderValue = OffsetBorderValue(relativeBorderValue);
            m_manipulator->setPixelPosition(absoluteBorderValue);
            SetBorderValue(m_sprite, m_border, absoluteBorderValue, totalUnscaledSizeInPixels, m_currentCellIndex);
        });

    QObject::connect(
        borderEditor,
        &SpriteBorderEditor::SelectedCellChanged,
        this,
        [this]([[maybe_unused]] ISprite* sprite, AZ::u32 index)
        {
            m_currentCellIndex = index;
        });
}

void SlicerEdit::SetManipulator(SlicerManipulator* manipulator)
{
    m_manipulator = manipulator;
}

void SlicerEdit::setPixelPosition(float p)
{
    // The border values should be presented to the user as offsets from
    // their corresponding borders. The given pixel position is expressed
    // in terms of total image size, so for Top and Left borders, the given
    // "pixel" position is indeed the distance from those borders. But for
    // Right and Bottom, we need to subtract the pixel postion from the
    // width and height of the image size (respectively) to present the
    // values as offsets from their respective borders.

    float relativeBorderValue = OffsetBorderValue(p);

    // For relatively small differences most likely due to floating point
    // inaccuracy, lock the difference to zero.
    const float epsilon = 0.001f;
    relativeBorderValue = relativeBorderValue >= epsilon ? relativeBorderValue : 0.0f;
    
    setText(QString::number(relativeBorderValue, 'f', 1));
}

float SlicerEdit::OffsetBorderValue(float borderValue) const
{
    const AZ::Vector2 cellSize = m_sprite->GetCellSize(m_currentCellIndex);

    if (SpriteBorder::Right == m_border)
    {
        return cellSize.GetX() - borderValue;
    }
    else if (SpriteBorder::Bottom == m_border)
    {
        return cellSize.GetY() - borderValue;
    }
    else
    {
        return borderValue;
    }
}

#include <moc_SlicerEdit.cpp>

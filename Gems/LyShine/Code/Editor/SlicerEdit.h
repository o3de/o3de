/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <LyShine/Bus/UiImageBus.h>
#include <QLineEdit>
#include "SpriteBorderEditorCommon.h"
#endif

//! Text input field that corresponds to a specific border value of a sprite image.
//!
//! These "border values" are used for 9-slicing a sprite.
//!
//! The "sprite" could be a specific cell inside of a spritesheet (border values
//! can be manipulated for individual cells within a spritesheet). 
class SlicerEdit
    : public QLineEdit
{
    Q_OBJECT

public:

    SlicerEdit(
        SpriteBorderEditor* borderEditor,
        SpriteBorder border,
        QSize& unscaledPixmapSize,
        ISprite* sprite);

    ~SlicerEdit() { }

    void SetManipulator(SlicerManipulator* manipulator);

    void setPixelPosition(float p);

private: // methods

    //! Calculates a border value relative to the max border value.
    //!
    //! For Right and Bottom border values, we want to present the
    //! border values as being relative to the border edge (that is,
    //! maximum value for the border). This aligns the value 
    //! presentation with the Left and Top border values, whose values
    //! are already "relative" to their borders (min border value, which
    //! is zero).
    //!
    //! This is interchangeably used to convert to and from relative
    //! border values, since both of the follow are true:
    //! relativeBorderValue = BorderMaxValue - absoluteValue
    //! absoluteValue = BorderMaxValue - relativeBorderValue
    //!
    //! For SlicerEdit fields associated with Left and Top border
    //! values, this is merely a pass-through function.
    float OffsetBorderValue(float borderValue) const;

private: // members

    SlicerManipulator* m_manipulator = nullptr;                     //!< Used to update on-screen manipulator position when the users
                                                                    //!< types in a value.

    ISprite* m_sprite;                                              //!< Sprite associated with this field

    SpriteBorder m_border;                                          //!< The sprite border that this input field is associated with.

    AZ::u32 m_currentCellIndex = 0;                                 //!< The sprite-sheet cell this border field field corresponds to
                                                                    //!< within the sprite-sheet (if applicable).
};

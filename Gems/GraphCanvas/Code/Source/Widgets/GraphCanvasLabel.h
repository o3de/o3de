/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QGraphicsWidget>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    //! The GraphCanvasLabel gives a QGraphicsWidget that is able to display text, and be placed into a layout.
    class GraphCanvasLabel
        : public QGraphicsWidget
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasLabel, AZ::SystemAllocator, 0);

        enum class WrapMode
        {
            MaximumWidth,
            BoundingWidth,
            ResizeToContent
        };

        enum class RoundedCornersMode
        {
            AllCorners,
            LeftCorners,
            RightCorners
        };

        GraphCanvasLabel(QGraphicsItem* parent = nullptr);
        ~GraphCanvasLabel() = default;

        void SetFontColor(const QColor& color);
        void ClearFontColor();

        void SetBorderColorOverride(const QBrush& borderOverride);
        const QBrush& GetBorderColorOverride() const;
        void ClearBorderColorOverride();

        void SetLabel(const AZStd::string& value);
        AZStd::string GetLabel() const { return AZStd::string(m_labelText.toUtf8().data()); }

        void SetSceneStyle(const AZ::EntityId& sceneId, const char* style);
        void SetStyle(const AZ::EntityId& entityId, const char* styleElement);
        void RefreshDisplay();

        void SetWrapMode(WrapMode wrapMode);

        //! Sets which corners to apply the radius to
        void SetRoundedCornersMode(RoundedCornersMode roundedCornerMode);

        QRectF GetDisplayedSize() const;

        //! Sets whether the text should elide if it grows beyond max-width
        //! (Note: currently incompatible with word wrap)
        void SetElide(bool elide);

        //! Sets whether the next should wrap if it grows beyond max-width
        //! (Note: currently incompatible with text elide)
        void SetWrap(bool wrap);

        //! Sets whether or not the text label will allow newlines in the text
        void SetAllowNewlines(bool allow);

        void SetDefaultAlignment(Qt::Alignment defaultAlignment);

        Styling::StyleHelper& GetStyleHelper();
        const Styling::StyleHelper& GetStyleHelper() const;

        void UpdateDisplayText();        

    protected:

        void UpdateDesiredBounds();

        // QGraphicsItem
        bool event(QEvent* qEvent) override;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        ////

        // QGraphicsLayoutItem
        QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint = QSizeF()) const override;
        ////

    private:        

        Qt::Alignment   m_defaultAlignment;
        bool m_elide;
        bool m_wrap;
        bool m_allowNewlines;

        QString         m_labelText;
        QString         m_displayText;

        QSizeF          m_maximumSize;
        QSizeF          m_minimumSize;

        //! So, this is going to return the actual value of boundingRect(as seen when we actually render).
        //! For whatever reason, preferredSize, boundingRect, size, rect, and even calling boundingRect internally and
        //! passing that value back does not give me the modified visual size, but instead the static preferred size.
        //! This means when we put this into something that scales it up(due to an expanding size policy)
        //! We get back improper values. So to combat this, we'll store the value when we render, and hope for the best.
        QRectF          m_displayedSize;

        //! This is influenced by the reflected value m_title.
        //! That's why we must update this value when m_title changes.
        QRectF m_desiredBounds;

        WrapMode m_wrapMode;
        RoundedCornersMode m_roundedCornersMode;

        bool                 m_hasBorderOverride;
        QBrush               m_borderColorOverride;

        Styling::StyleHelper m_styleHelper;
    };
}

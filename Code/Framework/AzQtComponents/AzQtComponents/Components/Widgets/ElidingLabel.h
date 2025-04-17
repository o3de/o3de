/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QLabel>
#endif

namespace AzQtComponents
{
    //! Extends the QLabel widget to automatically elide the label text with an
    //! ellipsis (...) if it doesn't fit within the width of the widget.
    //! On hover, a tooltip will show the full label text, plus an optional description.
    class AZ_QT_COMPONENTS_API ElidingLabel : public QLabel
    {
        Q_OBJECT
    public:
        explicit ElidingLabel(const QString& text, QWidget* parent = nullptr);
        explicit ElidingLabel(QWidget* parent = nullptr)
            : ElidingLabel("", parent)
        {
        }

        //! Sets the label text.
        void SetText(const QString& text)
        {
            setText(text);
        }
        //! Sets the label text.
        void setText(const QString& text);
        //! Returns the full label text.
        const QString& Text() const
        {
            return text();
        }
        //! Returns the full label text.
        const QString& text() const
        {
            return m_text;
        }
        //! Returns the elided text of the label as it is currently shown.
        const QString& ElidedText()
        {
            return elidedText();
        }
        //! Returns the elided text of the label as it is currently shown.
        const QString& elidedText()
        {
            return m_elidedText;
        }

        //! Set the label's filter string.
        void SetFilter(const QString& filter)
        {
            setFilter(filter);
        }
        //! Set the label's filter string.
        //! If the filter string is a substring of the current text, it will appear highlighted
        //! in the label. Used in conjunction with search filters to highlight results.
        void setFilter(const QString& filter);
        //! Returns if the label text matches the current filter string.
        //! If no filter string has been set, returns true.
        bool TextMatchesFilter() const;

        //! Sets the description for this label.
        void SetDescription(const QString& description)
        {
            setDescription(description);
        }
        //! Sets the description for this label.
        //! The description is shown in the label's tooltip, either on its own or
        //! alongside the full text in case of elision.
        void setDescription(const QString& description);
        //! Returns the description for this label.
        const QString& Description() const
        {
            return description();
        }
        //! Returns the description for this label.
        const QString& description() const
        {
            return m_description;
        }

        //! Sets the location of the ellipsis when eliding label text.
        void SetElideMode(Qt::TextElideMode mode)
        {
            setElideMode(mode);
        }
        //! Sets the location of the ellipsis when eliding label text.
        void setElideMode(Qt::TextElideMode mode);

        //! Refreshes the style on the label.
        void RefreshStyle()
        {
            refreshStyle();
        }
        //! Refreshes the style on the label.
        void refreshStyle();

        //! Sets the object name for the label.
        void setObjectName(const QString& name);
        //! Overrides the QLabel minimumSizeHint function to allow it to be shrunk further than its content.
        QSize minimumSizeHint() const override;
        //! Overrides the QLabel sizeHint function to return the elided text size.
        QSize sizeHint() const override;

        void handleElision();

    protected:
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void timerEvent(QTimerEvent* event) override;

        void requestElide(bool updateGeometry);

        QString m_filterString;
        QRegExp m_filterRegex;

    private:
        void elide();
        QRect TextRect();

        const QColor backgroundColor{ "#707070" };

        QString m_text;
        QString m_elidedText;
        QString m_description;

        Qt::TextElideMode m_elideMode;
        QLabel* m_metricsLabel;
        
        static constexpr int s_minTimeBetweenUpdates = 200;
        int m_elideTimerId = 0;
        bool m_elideDeferred = false;
        bool m_updateGeomentry = false;
    };

} // namespace AzQtComponents

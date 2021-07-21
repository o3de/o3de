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
#include <QWidget>
#endif

class QLineEdit;
class QLabel;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ColorHexEdit
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(qreal red READ red WRITE setRed NOTIFY redChanged)
        Q_PROPERTY(qreal green READ green WRITE setGreen NOTIFY greenChanged)
        Q_PROPERTY(qreal blue READ blue WRITE setBlue NOTIFY blueChanged)
        Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)

    public:
        struct ParsedColor
        {
            qreal red;
            qreal green;
            qreal blue;
            qreal alpha;
        };

        explicit ColorHexEdit(QWidget* parent = nullptr);
        ~ColorHexEdit() override;

        qreal red() const;
        qreal green() const;
        qreal blue() const;
        qreal alpha() const;

        bool editAlpha() const { return m_editAlpha; }

        void setLabelVisible(bool visible);

        bool eventFilter(QObject* watched, QEvent* event) override;

        static ParsedColor convertTextToColorValues(const QString& text, bool editAlpha, qreal fallbackAlpha);

    Q_SIGNALS:
        void redChanged(qreal red);
        void greenChanged(qreal green);
        void blueChanged(qreal blue);
        void alphaChanged(qreal alpha);
        void valueChangeBegan();
        void valueChangeEnded();

    public Q_SLOTS:
        void setRed(qreal red);
        void setGreen(qreal green);
        void setBlue(qreal blue);
        void setAlpha (qreal alpha);
        void setEditAlpha(bool editAlpha) { m_editAlpha = editAlpha; }

    private Q_SLOTS:
        void textChanged(const QString& text);

    private:
        void initEditValue();
        void showContextMenu(const QPoint& pos);

        qreal m_red;
        qreal m_green;
        qreal m_blue;
        qreal m_alpha;

        QLineEdit* m_edit;
        QLabel* m_hexLabel;

        bool m_valueChanging = false;
        bool m_editAlpha = false;
    };
} // namespace AzQtComponents

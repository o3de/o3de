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
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#endif

class QLabel;
class QSvgWidget;
namespace AzQtComponents
{
    class Style;
    // Widget to display an animated progress GIF
    class AZ_QT_COMPONENTS_API StyledBusyLabel
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit StyledBusyLabel(QWidget* parent = nullptr);

        bool GetIsBusy() const;
        void SetIsBusy(bool busy);

        QString GetText() const;
        void SetText(const QString& text);

        QString GetBusyIcon() const;
        void SetBusyIcon(const QString& iconSource);

        int GetBusyIconSize() const;
        void SetBusyIconSize(int iconSize);

        QSize sizeHint() const override;

        void SetUseNewWidget(bool usenew);

        void DrawTo(QPainter* painter, const QRectF& bounds) const;

        QPixmap GetPixmap(QSize size);
    signals:
        void repaintNeeded();

    public slots:
        void movieUpdated();

    private:
        friend class Style;
        void updateMovie();
        void loadDefaultIcon();
        static bool polish(Style* style, QWidget* widget);
        static bool unpolish(Style* style, QWidget* widget);

        bool m_isBusy = false;
        int m_busyIconSize = 32;
        QString m_fileName;
        QSvgWidget* m_busyIcon = nullptr;
        QLabel* m_oldBusyIcon = nullptr;
        QLabel* m_text;
        bool m_useNewWidget = false;
    };
} // namespace AzQtComponents

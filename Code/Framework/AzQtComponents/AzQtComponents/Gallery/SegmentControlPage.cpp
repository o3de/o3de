/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SegmentControlPage.h"
#include <AzQtComponents/Gallery/ui_SegmentControlPage.h>

#include <AzQtComponents/Components/Widgets/SegmentBar.h>
#include <AzQtComponents/Components/Widgets/SegmentControl.h>

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>

class SampleColorSwatch
    : public QWidget
{
public:
    SampleColorSwatch(const QColor& color, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_color(color)
    {
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.setPen(Qt::NoPen);
        painter.setBrush(m_color);
        painter.drawRect(contentsRect());
    }

private:
    QColor m_color;
};

SegmentControlPage::SegmentControlPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::SegmentControlPage)
{
    ui->setupUi(this);
    ui->segmentBarVertical->setOrientation(Qt::Vertical);
    ui->segmentControlVertical->setTabOrientation(Qt::Vertical);
    ui->segmentControlVertical->setTabPosition(AzQtComponents::SegmentControl::West);

    // set up a segment bar
    const QVector<AzQtComponents::SegmentBar*> bars{ ui->segmentBar,
                                                     ui->segmentBarVertical };
    for (AzQtComponents::SegmentBar* sb : bars)
    {
        for (int i = 0; i < 4; i++)
        {
            sb->addTab(QStringLiteral("Segment Bar %1").arg(i));
        }
    }

    // set up a segment control
    const QVector<AzQtComponents::SegmentControl*> controls{ ui->segmentControl,
                                                             ui->segmentControlVertical };
    for (AzQtComponents::SegmentControl* sc : controls)
    {
        sc->addTab(new SampleColorSwatch(QColor("#690000"), sc), "Red");
        sc->addTab(new SampleColorSwatch(QColor("#006900"), sc), "Green");
        sc->addTab(new SampleColorSwatch(QColor("#000069"), sc), "Blue");
    }

    QString exampleText = R"(

TabWidget docs: <a href="http://doc.qt.io/qt-5/qtabwidget.html">http://doc.qt.io/qt-5/qtabwidget.html</a><br/>
TabBar docs: <a href="http://doc.qt.io/qt-5/qtabbar.html">http://doc.qt.io/qt-5/qtabbar.html</a><br/>

<br/>
The Segment Control and Segment Bar can be used interchangeably with the QTabWidget and QTabBar.
<br/>
<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/SegmentControl.h&gt;
#include &lt;QStackedWidget&gt;
#include &lt;QDebug&gt;

// Create a new Segment Control:
AzQtComponents::SegmentControl* segmentControl = new AzQtComponents::SegmentControl(this);

// This can also be done in a .ui file

// To add sub-tab widgets to the control:
for (int i = 0; i &lt; 4; i++)
{
    segmentControl->addTab(new QWidget(segmentControl), QStringLiteral("New Tab %1").arg(i));
}

// If you just want a Segment Bar and to control what gets displayed and when, do this:
AzQtComponents::SegmentBar* segmentBar = new AzQtComponents::SegmentBar(this);
QStackedWidget* stackedWidget = new QStackedWidget(this);

layout()->addWidget(segmentBar);
layout()->addWidget(stackedWidget);

for (int i = 0; i &lt; 4; i++)
{
    segmentBar->addTab(QStringLiteral("New Tab %1").arg(i));
    stackedWidget->addTab(new QWidget(segmentControl));
}

connect(segmentBar, &AzQtComponents::SegmentBar::currentChanged, [](int newIndex){
    stackedWidget->setCurrentIndex(newIndex);
});

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

SegmentControlPage::~SegmentControlPage()
{
}

#include "Gallery/moc_SegmentControlPage.cpp"

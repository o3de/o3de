/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SvgLabelPage.h"
#include <AzQtComponents/Gallery/ui_SvgLabelPage.h>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QSvgRenderer>

SvgLabelPage::SvgLabelPage(QWidget* parent) 
    : QWidget(parent)
    , m_ui(new Ui::SvgLabelPage)
{
    m_ui->setupUi(this);
    QObject::connect(m_ui->widthBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SvgLabelPage::onWidthChanged);
    QObject::connect(m_ui->heightBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SvgLabelPage::onHeightChanged);
    QObject::connect(m_ui->resetSize, &QPushButton::clicked, this, &SvgLabelPage::onResetSize);
    QObject::connect(m_ui->maintainAspectRatio, &QCheckBox::toggled, this, &SvgLabelPage::onMaintainAspectToggled);
    setAcceptDrops(true);

    QString exampleText = R"(

This is a simple QSvgWidget that demonstrates how Qt renders scalable vector graphics (svg files).<br/><br>
Drag and drop a .svg file onto this page to load and display the image. You can also manipulate the width and height of the rendered svg using the controls on the right.<br/>
Qt SVG docs: <a href="https://doc.qt.io/qt-5.10/qtsvg-module.html">https://doc.qt.io/qt-5.10/qtsvg-module.html</a><br/>

<pre>
#include &lt;QSvgRenderer&gt;
#include &lt;QSvgWidget&gt;

// widget to show an svg image:
QSvgWidget theImage;
theImage.load("pathToFile/SvgFile.svg");
<br>

// class to render an svg file to an image:
QSvgRenderer theRenderer;
theRenderer.load("pathToFile/SvgFile.svg");
theRenderer.render(theQPainter);

</pre>

)";

    m_ui->exampleText->setHtml(exampleText);
}

SvgLabelPage::~SvgLabelPage()
{

}

void SvgLabelPage::onWidthChanged(int newWidth)
{
    m_ui->image->setMinimumWidth(newWidth);
    m_ui->image->setMaximumWidth(newWidth);

    if (m_ui->maintainAspectRatio->isChecked())
    {
        QSignalBlocker blockResize(m_ui->image);
        const int newHeight = qRound(float(newWidth) * (float(m_initialSize.height()) / m_initialSize.width()));
        m_ui->heightBox->setValue(newHeight);
    }
}

void SvgLabelPage::onHeightChanged(int newHeight)
{
    
    m_ui->image->setMinimumHeight(newHeight);
    m_ui->image->setMaximumHeight(newHeight);

    if (m_ui->maintainAspectRatio->isChecked())
    {
        QSignalBlocker blockResize(m_ui->image);
        const int newWidth = qRound(float(newHeight) * (float(m_initialSize.width()) / m_initialSize.height()));
        m_ui->widthBox->setValue(newWidth);
    }
}

void SvgLabelPage::onResetSize()
{
    m_ui->widthBox->setValue(m_initialSize.width());
    m_ui->heightBox->setValue(m_initialSize.height());
}

void SvgLabelPage::onMaintainAspectToggled(bool checked)
{
    // if they've checked maintain aspect ratio, adjust the height to be proportional to the width
    if (checked)
    {
        onWidthChanged(m_ui->widthBox->value());
    }
}

void SvgLabelPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void SvgLabelPage::dropEvent(QDropEvent *event)
{
    Q_ASSERT(event->mimeData()->hasUrls());
    QUrl firstUrl = event->mimeData()->urls().first();
    QSvgWidget* theImage = m_ui->image;
    theImage->load(firstUrl.toLocalFile());
    m_initialSize = theImage->renderer()->defaultSize();
    onResetSize();
}

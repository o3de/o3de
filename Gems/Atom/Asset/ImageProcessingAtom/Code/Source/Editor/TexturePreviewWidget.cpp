/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageProcessing_precompiled.h>
#include "TexturePreviewWidget.h"
#include <Processing/PixelFormatInfo.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QSize>
#include <QPoint>
#include <QPixmap>
#include <QPainter>
#include <QComboBox>
#include <QColor>
#include <QEvent>
#include <QKeyEvent>
#include <QApplicationStateChangeEvent>
#include <QString>
#include <QMenu>
#include <QWidgetAction>
#include <Source/Editor/ui_TexturePreviewWidget.h>
AZ_POP_DISABLE_WARNING

namespace ImageProcessingAtomEditor
{
    using namespace ImageProcessingAtom;

    TexturePreviewWidget::TexturePreviewWidget(EditorTextureSetting& texureSetting, QWidget* parent /*= nullptr*/)
        : QWidget(parent)
        , m_ui(new Ui::TexturePreviewWidget)
        , m_textureSetting(&texureSetting)
    {
        m_ui->setupUi(this);

        m_platform = BuilderSettingManager::s_defaultPlatform;
        // For now, only provide preview for default platform
        m_previewConverter = AZStd::make_unique<ImageProcessingAtom::ImagePreview>(m_textureSetting->m_fullPath, &m_textureSetting->GetMultiplatformTextureSetting());

        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &TexturePreviewWidget::UpdatePreview);
        m_updateTimer->setSingleShot(false);

        m_ui->infoLayer->setAttribute(Qt::WA_NoSystemBackground);
        m_ui->mipLevelLabel->setAttribute(Qt::WA_NoSystemBackground);
        m_ui->imageSizeLabel->setAttribute(Qt::WA_NoSystemBackground);
        m_ui->fileSizeLabel->setAttribute(Qt::WA_NoSystemBackground);

        // Setup preview mode combo box
        static const QString previewModeString[] = { "RGB",
                                                     "R",
                                                     "G",
                                                     "B",
                                                     "Alpha",
                                                     "RGBA" };

        for (int i = 0; i < (int)PreviewMode::Count; i++)
        {
            m_ui->previewComboBox->addItem(previewModeString[i]);
        }

        QSize size = m_ui->imageLabel->size();
        m_imageLabelSize = static_cast<float>(size.width());

        SetUpResolutionInfo();
        RefreshUI(true);

        QObject::connect(m_ui->previewCheckBox, &QCheckBox::clicked, this, &TexturePreviewWidget::OnTiledChanged);
        QObject::connect(m_ui->nextMipBtn, &QPushButton::clicked, this, &TexturePreviewWidget::OnNextMip);
        QObject::connect(m_ui->prevMipBtn, &QPushButton::clicked, this, &TexturePreviewWidget::OnPrevMip);
        QObject::connect(m_ui->previewComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TexturePreviewWidget::OnChangePreviewMode);

        // Set up Refresh button
        m_alwaysRefreshAction = new QAction("Always refresh preview", this);
        m_alwaysRefreshAction->setCheckable(true);
        m_alwaysRefreshAction->setChecked(m_alwaysRefreshPreview);
        QObject::connect(m_alwaysRefreshAction, &QAction::triggered, this, &TexturePreviewWidget::OnAlwaysRefresh);

        m_refreshPerClickAction = new QAction("Press to refresh preview", this);
        m_refreshPerClickAction->setCheckable(true);
        m_refreshPerClickAction->setChecked(!m_alwaysRefreshPreview);
        QObject::connect(m_refreshPerClickAction, &QAction::triggered, this, &TexturePreviewWidget::OnRefreshPerClick);

        QMenu* menu = new QMenu(this);
        menu->addAction(m_alwaysRefreshAction);
        menu->addAction(m_refreshPerClickAction);

        m_ui->refreshBtn->setMenu(menu);
        AzQtComponents::PushButton::applySmallIconStyle(m_ui->refreshBtn);

        QObject::connect(m_ui->refreshBtn, &QPushButton::clicked, this, &TexturePreviewWidget::OnRefreshClicked);
        m_alwaysRefreshIcon.addFile(QStringLiteral(":/refresh.png"), QSize(), QIcon::Normal, QIcon::On);
        m_refreshPerClickIcon.addFile(QStringLiteral(":/refresh-active.png"), QSize(), QIcon::Normal, QIcon::On);
        m_ui->refreshBtn->setIcon(m_alwaysRefreshIcon);

        m_ui->busyLabel->SetBusyIconSize(16);
        SetImageLabelText(QString(), false);

        // Tooltips
        m_ui->previewComboBox->setToolTip(QString("Preview the texture in different channels."));
        m_ui->previewCheckBox->setToolTip(QString("Show or hide a 2x2 tiling of the texture."));
        m_ui->hotkeyLabel->setToolTip(QString("Preview different texture states with keyboard shortcuts."));
        m_ui->refreshBtn->setToolTip(QString("Provide different ways to refresh the preview. Click on the button to refresh manually."));

        EditorInternalNotificationBus::Handler::BusConnect();
    }

    TexturePreviewWidget::~TexturePreviewWidget()
    {
        EditorInternalNotificationBus::Handler::BusDisconnect();
    }

    void TexturePreviewWidget::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);

        QSize size = m_ui->mainWidget->size();
        m_ui->infoLayer->resize(size);

        QSize imageSize = m_ui->imageLabel->size();
        QPoint center = m_ui->mainWidget->rect().center();
        m_ui->imageLabel->move(center - QPoint(imageSize.width() / 2, imageSize.height() / 2));
        QSize busyLabelSize = m_ui->busyLabel->size();
        m_ui->busyLabel->move(center - QPoint(busyLabelSize.width() + m_ui->imageLabel->sizeHint().width() / 2, busyLabelSize.width() / 2));
    }

    void TexturePreviewWidget::SetUpResolutionInfo()
    {
        m_resolutionInfos = m_textureSetting->GetResolutionInfoForMipmap(m_platform);
        m_mipCount = (unsigned int)m_resolutionInfos.size();
        if (m_currentMipIndex > (int)m_mipCount)
        {
            m_currentMipIndex = 0;
        }
    }

    void TexturePreviewWidget::OnEditorSettingsChanged([[maybe_unused]] bool needRefresh, const AZStd::string& platform)
    {
        // Only update the preview if there is any change in current platform
        if (platform == m_platform)
        {
            SetUpResolutionInfo();
            RefreshUI(true);
        }
    }

    void TexturePreviewWidget::RefreshUI(bool fullRefresh)
    {
        m_ui->mipLevelLabel->setText(QString("Mip %1").arg(QString::number(m_currentMipIndex)));
        m_ui->previewCheckBox->setCheckState(m_previewTiled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

        bool hasNextMip = m_currentMipIndex < (int)m_mipCount - 1;
        m_ui->nextMipBtn->setVisible(hasNextMip);

        bool hasPrevMip = m_currentMipIndex > 0;
        m_ui->prevMipBtn->setVisible(hasPrevMip);

        RefreshWarning();

        if (m_currentMipIndex < m_resolutionInfos.size())
        {
            auto it = AZStd::next(m_resolutionInfos.begin(), m_currentMipIndex);
            QString finalResolution;
            if (it->arrayCount > 1)
            {
                finalResolution = QString("Image Size: %1 x %2 x %3").arg(QString::number(it->width), QString::number(it->height), QString::number(it->arrayCount));
            }
            else
            {
                finalResolution = QString("Image Size: %1 x %2").arg(QString::number(it->width), QString::number(it->height));
            }
            m_ui->imageSizeLabel->setText(finalResolution);
            
            CPixelFormats& pixelFormats = CPixelFormats::GetInstance();
            const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(m_textureSetting->GetMultiplatformTextureSetting().m_preset);
            if (preset)
            {
                float size = static_cast<float>(pixelFormats.EvaluateImageDataSize(preset->m_pixelFormat, it->width, it->height));
                AZStd::string fileSizeString = EditorHelper::GetFileSizeString(static_cast<AZ::u32>(size));
                QString finalFileSize = QString("File Size: %1").arg(fileSizeString.c_str());
                m_ui->fileSizeLabel->setText(finalFileSize);
            }

            if (m_alwaysRefreshPreview)
            {
                RefreshPreviewImage(fullRefresh ? RefreshMode::Convert : RefreshMode::Mip);
            }
        }
        else
        {
            AZ_Error("Texture Setting", false, "Cannot find mip reduce level for mip %d", m_currentMipIndex);
        }
    }

    void TexturePreviewWidget::OnNextMip()
    {
        if (m_currentMipIndex >= (int)m_mipCount - 1)
        {
            return;
        }
        m_currentMipIndex++;
        RefreshUI(false);
    }


    void TexturePreviewWidget::OnPrevMip()
    {
        if (m_currentMipIndex <= 0)
        {
            return;
        }

        m_currentMipIndex--;
        RefreshUI(false);
    }

    void TexturePreviewWidget::UpdatePreview()
    {
        if (!m_previewConverter->IsDone())
        {
            float progress = m_previewConverter->GetProgress();
            SetImageLabelText(QString("Converting for preview...Progress %1%").arg(QString::number(progress * 100, 'f', 2)));
            return;
        }

        m_updateTimer->stop();
        m_previewImageRaw = m_previewConverter->GetOutputImage();

        GenerateMipmap(m_currentMipIndex);
        GenerateChannelImage(m_previewMode);
        PaintPreviewImage();
    }

    void TexturePreviewWidget::OnAlwaysRefresh()
    {
        m_alwaysRefreshPreview = true;
        m_alwaysRefreshAction->setChecked(true);
        m_refreshPerClickAction->setChecked(false);

        m_ui->refreshBtn->setIcon(m_alwaysRefreshIcon);
    }

    void TexturePreviewWidget::OnRefreshPerClick()
    {
        m_alwaysRefreshPreview = false;
        m_alwaysRefreshAction->setChecked(false);
        m_refreshPerClickAction->setChecked(true);

        m_ui->refreshBtn->setIcon(m_refreshPerClickIcon);
    }

    void TexturePreviewWidget::OnRefreshClicked()
    {
        RefreshPreviewImage(RefreshMode::Convert);
    }

    void TexturePreviewWidget::GenerateMipmap(int mip)
    {
        // Clear all cached preview images
        for (int i = 0; i < (int)PreviewMode::Count; i++)
        {
            m_previewImages[i] = QImage();
        }

        if (m_previewImageRaw && (AZ::u32)mip < m_previewImageRaw->GetMipCount())
        {
            uint8* imageBuf;
            uint32 pitch;
            m_previewImageRaw->GetImagePointer(mip, imageBuf, pitch);
            const uint32 width = m_previewImageRaw->GetWidth(mip);
            const uint32 height = m_previewImageRaw->GetHeight(mip);
            m_previewImages[PreviewMode::RGBA] = QImage(imageBuf, width, height, pitch, QImage::Format_RGBA8888);
        }
        else
        {
            AZ_Error("Texture Editor", false, "Cannot generate mip preview from an invalid image.");
        }
    }

    void TexturePreviewWidget::GenerateChannelImage(PreviewMode channel)
    {
        // If there is no preview image generated, ignore this function
        if (m_previewImages[PreviewMode::RGBA].isNull())
        {
            AZ_Error("Texture Editor", false, "Cannot generate channel image from an invalid image.");
            return;
        }

        if (m_previewImages[channel].isNull())
        {
            // Copy the RGBA image before changing the color
            QImage previewImg = m_previewImages[PreviewMode::RGBA].copy();
            for (int x = 0; x < previewImg.width(); x++)
            {
                for (int y = 0; y < previewImg.height(); y++)
                {
                    QRgb pixel = previewImg.pixel(x, y);
                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);
                    int a = qAlpha(pixel);

                    switch (channel)
                    {
                    case ImageProcessingAtomEditor::RGB:
                        pixel = qRgba(r, g, b, 255);
                        break;
                    case ImageProcessingAtomEditor::RRR:
                        pixel = qRgba(r, r, r, 255);
                        break;
                    case ImageProcessingAtomEditor::GGG:
                        pixel = qRgba(g, g, g, 255);
                        break;
                    case ImageProcessingAtomEditor::BBB:
                        pixel = qRgba(b, b, b, 255);
                        break;
                    case ImageProcessingAtomEditor::Alpha:
                        pixel = qRgba(a, a, a, 255);
                        break;
                    default:
                        break;
                    }

                    previewImg.setPixel(x, y, pixel);
                }
            }
            // Cache the image in current preview mode
            m_previewImages[channel] = previewImg;
        }
    }

    void TexturePreviewWidget::RefreshPreviewImage(RefreshMode mode)
    {
        // Ignore any none-conversion refresh request when the image is being converted
        if (m_updateTimer->isActive() && mode != RefreshMode::Convert)
        {
            return;
        }

        switch (mode)
        {
        case RefreshMode::Convert:
        {
            // Start conversion in a AZ::Job
            m_previewConverter->StartConvert();
            // Start the timer to trigger the update function
            m_updateTimer->start(s_updateInterval);
            SetImageLabelText(QString("Converting for preview...Progress 0.01%"));
        }
        break;
        case RefreshMode::Mip:
        {
            GenerateMipmap(m_currentMipIndex);
            GenerateChannelImage(m_previewMode);
            PaintPreviewImage();
        }
        break;
        case RefreshMode::Channel:
        {
            GenerateChannelImage(m_previewMode);
            PaintPreviewImage();
        }
        break;
        default:
            PaintPreviewImage();
            break;
        }
    }

    void TexturePreviewWidget::PaintPreviewImage()
    {
        if (m_previewImages[m_previewMode].isNull())
        {
            SetImageLabelText(QString("Conversion failed, please check console for more information."), false);
            return;
        }
        SetImageLabelText(QString(), false);

        // Paint the image on to the image label
        QPixmap pixMap = QPixmap::fromImage(m_previewImages[m_previewMode]);
        QSize size = m_ui->imageLabel->size();
        QPixmap finalPix = pixMap.copy();
        finalPix.fill(Qt::transparent);
        finalPix = finalPix.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPainter painter(&finalPix);
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        QRect rect = finalPix.rect();
        if (m_previewTiled)
        {
            pixMap = pixMap.scaled(size / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            painter.drawTiledPixmap(rect, pixMap);
        }
        else
        {
            painter.drawPixmap(rect, pixMap);
        }
        // Recenter the image label
        float aspectRatio = static_cast<float>(finalPix.width()) / static_cast<float>(finalPix.height());
        QSize preferredSize;
        if (aspectRatio >= 1.0f)
        {
            preferredSize = QSize(static_cast<int>(m_imageLabelSize), static_cast<int>(m_imageLabelSize / aspectRatio));
        }
        else
        {
            preferredSize = QSize(static_cast<int>(m_imageLabelSize * aspectRatio), static_cast<int>(m_imageLabelSize));
        }

        m_ui->imageLabel->resize(preferredSize);
        m_ui->imageLabel->setPixmap(finalPix);

        QPoint center = m_ui->mainWidget->rect().center();
        m_ui->imageLabel->move(center - QPoint(preferredSize.width() / 2, preferredSize.height() / 2));
    }

    void TexturePreviewWidget::SetImageLabelText(const QString& text, bool busyStatus /*= true*/)
    {
        // Since setting pixmap will change the label size
        // Need to set back to initial size and recenter before displaying text
        m_ui->imageLabel->resize(QSize(static_cast<int>(m_imageLabelSize), static_cast<int>(m_imageLabelSize)));
        QPoint center = m_ui->mainWidget->rect().center();
        m_ui->imageLabel->move(center - QPoint(static_cast<int>(m_imageLabelSize / 2.0f), static_cast<int>(m_imageLabelSize / 2.0f)));
        m_ui->imageLabel->setText(text);

        // Set busy label status and position to align with the text
        m_ui->busyLabel->SetIsBusy(busyStatus);
        QSize size = m_ui->busyLabel->size();
        m_ui->busyLabel->move(center - QPoint(size.width() + m_ui->imageLabel->sizeHint().width() / 2, size.width() / 2));
        m_ui->busyLabel->setVisible(busyStatus);
    }

    void TexturePreviewWidget::RefreshWarning()
    {
        int imageWidth = m_textureSetting->m_img->GetWidth(0);
        int imageHeight = m_textureSetting->m_img->GetHeight(0);
        AZStd::list<PlatformName> stretchedPlatform;

        for (auto& iter: m_textureSetting->m_settingsMap)
        {
            PlatformName platform = iter.first;
            const PresetSettings* presetSettings = BuilderSettingManager::Instance()->GetPreset(iter.second.m_preset, platform);
            if (presetSettings)
            {
                EPixelFormat dstFmt = presetSettings->m_pixelFormat;
                if (!CPixelFormats::GetInstance().IsImageSizeValid(dstFmt, imageWidth, imageHeight, false))
                {
                    stretchedPlatform.push_back(EditorHelper::ToReadablePlatformString(platform).c_str());
                }
            }
        }

        if (stretchedPlatform.size() > 0)
        {
            QString warningText = QString("The output image will be stretched on Platform:");
            int i = 0;
            for (AZStd::string platform: stretchedPlatform)
            {
                warningText += i > 0 ? ", " : " ";
                warningText += platform.c_str();
                i++;
            }
            m_ui->warningLabel->setText(warningText);
            m_ui->warningLabel->setVisible(true);
            m_ui->warningIcon->setVisible(true);
        }
        else
        {
            m_ui->warningLabel->setVisible(false);
            m_ui->warningIcon->setVisible(false);
        }
    }

    void TexturePreviewWidget::OnChangePreviewMode(int index)
    {
        if (index < (int)PreviewMode::Count)
        {
            m_previewMode = (PreviewMode)index;

            RefreshPreviewImage(RefreshMode::Channel);
        }
    }

    void TexturePreviewWidget::OnTiledChanged(bool checked)
    {
        m_previewTiled = checked;
        RefreshPreviewImage(RefreshMode::Repaint);
    }

    bool TexturePreviewWidget::OnQtEvent(QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            const QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->isAutoRepeat())
            {
                return false; //ignore repeat key event
            }

            if (ke->key() == Qt::Key_Space)
            {
                if (!m_updateTimer->isActive()) // Only popup when image is not converting
                {
                    m_previewPopup.reset(new ImagePopup(m_previewImages[m_previewMode], this));
                    m_previewPopup->installEventFilter(this);
                    m_previewPopup->show();
                    event->accept();
                    return true;
                }
            }
            else if (ke->key() == Qt::Key_Alt)
            {
                m_previewMode = PreviewMode::Alpha;
                RefreshPreviewImage(RefreshMode::Channel);
                event->accept();
                return true;
            }
            else if (ke->key() == Qt::Key_Shift)
            {
                m_previewMode = PreviewMode::RGBA;
                RefreshPreviewImage(RefreshMode::Channel);
                event->accept();
                return true;
            }
        }
        else if (event->type() == QEvent::KeyRelease)
        {
            const QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->isAutoRepeat())
            {
                return false; //ignore repeat key event
            }
            if (ke->key() == Qt::Key_Space)
            {
                if (m_previewPopup)
                {
                    m_previewPopup->hide();
                }
                event->accept();
                return true;
            }
            else if (ke->key() == Qt::Key_Alt)
            {
                m_previewMode = (PreviewMode)m_ui->previewComboBox->currentIndex();
                RefreshPreviewImage(RefreshMode::Channel);
                event->accept();
                return true;
            }
            else if (ke->key() == Qt::Key_Shift)
            {
                m_previewMode = (PreviewMode)m_ui->previewComboBox->currentIndex();
                RefreshPreviewImage(RefreshMode::Channel);
                event->accept();
                return true;
            }
        }
        else if (event->type() == QEvent::ApplicationStateChange)
        {
            const QApplicationStateChangeEvent* appEvent = static_cast<QApplicationStateChangeEvent*>(event);
            AZ_Warning("Texture Editor", false, "app status change %d", appEvent->applicationState());
            if (appEvent->applicationState() != Qt::ApplicationState::ApplicationActive)
            {
                PreviewMode currPreviewMode = (PreviewMode)m_ui->previewComboBox->currentIndex();
                if (m_previewMode != currPreviewMode)
                {
                    m_previewMode = currPreviewMode;
                    RefreshPreviewImage(RefreshMode::Channel);
                    event->accept();
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::ShortcutOverride)
        {
            // since we respond to the following things, let Qt know so that shortcuts don't override us

            QKeyEvent* kev = static_cast<QKeyEvent*>(event);
            int key = kev->key() | kev->modifiers();
            switch (key)
            {
            case Qt::Key_Space:
            case Qt::Key_Alt:
            case Qt::Key_Shift:
                event->accept();
                return true;
                break;

            default:
                break;
            }
        }
        return false;
    }

    bool TexturePreviewWidget::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::KeyRelease)
        {
            const QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Space && !ke->isAutoRepeat())
            {
                if (m_previewPopup)
                {
                    m_previewPopup->hide();
                }
                return true;
            }
        }
        else if (event->type() == QEvent::ApplicationStateChange)
        {
            const QApplicationStateChangeEvent* appEvent = static_cast<QApplicationStateChangeEvent*>(event);
            if (appEvent->applicationState() != Qt::ApplicationState::ApplicationActive)
            {
                if (m_previewPopup)
                {
                    m_previewPopup->hide();
                }
            }
            return true;
        }

        return QWidget::eventFilter(obj, event);
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_TexturePreviewWidget.cpp>

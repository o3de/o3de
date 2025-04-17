/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemItemDelegate.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <AdjustableHeaderWidget.h>
#include <ProjectManagerDefs.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QEvent>
#include <QAbstractItemView>
#include <QPainter>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QHoverEvent>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QMovie>
#include <QHeaderView>
#include <QDir>

namespace O3DE::ProjectManager
{
    GemItemDelegate::GemItemDelegate(QAbstractItemModel* model, AdjustableHeaderWidget* header, bool readOnly, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_model(model)
        , m_headerWidget(header)
        , m_readOnly(readOnly)
    {
        AddPlatformIcon(GemInfo::Android, ":/Android.svg");
        AddPlatformIcon(GemInfo::iOS, ":/iOS.svg");
        AddPlatformIcon(GemInfo::Linux, ":/Linux.svg");
        AddPlatformIcon(GemInfo::macOS, ":/macOS.svg");
        AddPlatformIcon(GemInfo::Windows, ":/Windows.svg");

        SetStatusIcon(m_notDownloadedPixmap, ":/Download.svg");
        SetStatusIcon(m_unknownStatusPixmap, ":/X.svg");
        SetStatusIcon(m_downloadSuccessfulPixmap, ":/checkmark.svg");
        SetStatusIcon(m_downloadFailedPixmap, ":/Warning.svg");
        SetStatusIcon(m_downloadedPixmap, ":/Downloaded.svg");

        m_updatePixmap = QIcon(":/Update.svg").pixmap(s_statusIconSize, s_statusIconSize);

        m_downloadingMovie = new QMovie(":/in_progress.gif");
    }

    void GemItemDelegate::AddPlatformIcon(GemInfo::Platform platform, const QString& iconPath)
    {
        QPixmap pixmap(iconPath);
        qreal aspectRatio = static_cast<qreal>(pixmap.width()) / pixmap.height();
        m_platformIcons.insert(platform, QIcon(iconPath).pixmap(static_cast<int>(static_cast<qreal>(s_platformIconSize) * aspectRatio), s_platformIconSize));
    }

    void GemItemDelegate::SetStatusIcon(QPixmap& m_iconPixmap, const QString& iconPath)
    {
        QPixmap pixmap(iconPath);
        const float aspectRatio = static_cast<float>(pixmap.width()) / pixmap.height();
        int xScaler = m_readOnly ? s_statusIconSizeLarge : s_statusIconSize;
        int yScaler = xScaler;

        if (aspectRatio > 1.0f)
        {
            yScaler = static_cast<int>(1.0f / aspectRatio * xScaler);
        }
        else if (aspectRatio < 1.0f)
        {
            xScaler = static_cast<int>(aspectRatio * yScaler);
        }

        m_iconPixmap = QIcon(iconPath).pixmap(xScaler, yScaler);
    }

    void GemItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return;
        }

        const GemInfo& gemInfo = GemModel::GetGemInfo(modelIndex);

        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        painter->setRenderHint(QPainter::Antialiasing);

        QRect fullRect, itemRect, contentRect;
        CalcRects(options, fullRect, itemRect, contentRect);

        QFont standardFont(options.font);
        standardFont.setPixelSize(static_cast<int>(s_fontSize));
        QFontMetrics standardFontMetrics(standardFont);

        painter->save();
        painter->setClipping(true);
        painter->setClipRect(fullRect);
        painter->setFont(options.font);

        // Draw background
        painter->fillRect(fullRect, m_backgroundColor);

        // Draw item background
        const QColor itemBackgroundColor = options.state & QStyle::State_MouseOver ? m_itemBackgroundColor.lighter(120) : m_itemBackgroundColor;
        painter->fillRect(itemRect, itemBackgroundColor);

        // Draw border
        if (options.state & QStyle::State_Selected)
        {
            painter->save();
            QPen borderPen(m_borderColor);
            borderPen.setWidth(s_borderWidth);
            painter->setPen(borderPen);
            painter->drawRect(itemRect);
            painter->restore();
        }

        // Gem preview
        QString previewPath = QDir(gemInfo.m_path).filePath(ProjectPreviewImagePath);
        QPixmap gemPreviewImage(previewPath);
        QRect gemPreviewRect(
            contentRect.left() + AdjustableHeaderWidget::s_headerTextIndent,
            contentRect.center().y() - GemPreviewImageHeight / 2,
            GemPreviewImageWidth, GemPreviewImageHeight);
        painter->drawPixmap(gemPreviewRect, gemPreviewImage);

        // Gem (dispay) name
        QFont gemNameFont(options.font);
        QPair<int, int> nameXBounds = CalcColumnXBounds(HeaderOrder::Name);
        const int nameStartX = nameXBounds.first;
        const int nameColumnTextStartX = s_itemMargins.left() + nameStartX + AdjustableHeaderWidget::s_headerTextIndent;
        const int nameColumnMaxTextWidth = nameXBounds.second - nameStartX - AdjustableHeaderWidget::s_headerTextIndent;
        gemNameFont.setPixelSize(static_cast<int>(s_gemNameFontSize));
        gemNameFont.setBold(true);
        QString gemName = QFontMetrics(gemNameFont).elidedText(GemModel::GetDisplayName(modelIndex), Qt::TextElideMode::ElideRight, nameColumnMaxTextWidth);
        QRect gemNameRect = GetTextRect(gemNameFont, gemName, s_gemNameFontSize);
        gemNameRect.moveTo(nameColumnTextStartX, contentRect.top());
        painter->setFont(gemNameFont);
        painter->setPen(m_textColor);
        gemNameRect = painter->boundingRect(gemNameRect, Qt::TextSingleLine, gemName);
        painter->drawText(gemNameRect, Qt::TextSingleLine, gemName);

        // Gem creator
        QString gemCreator = gemInfo.m_origin;
        gemCreator = standardFontMetrics.elidedText(gemCreator, Qt::TextElideMode::ElideRight, nameColumnMaxTextWidth);
        QRect gemCreatorRect = GetTextRect(standardFont, gemCreator, s_fontSize);
        gemCreatorRect.moveTo(nameColumnTextStartX, contentRect.top() + gemNameRect.height());

        painter->setFont(standardFont);
        gemCreatorRect = painter->boundingRect(gemCreatorRect, Qt::TextSingleLine, gemCreator);
        painter->drawText(gemCreatorRect, Qt::TextSingleLine, gemCreator);

        // Gem summary
        const bool hasTags = !gemInfo.m_features.isEmpty();
        const QRect summaryRect = CalcSummaryRect(contentRect, hasTags);
        DrawText(gemInfo.m_summary, painter, summaryRect, standardFont);

        // Gem Version
        if (!gemInfo.m_version.isEmpty() && !gemInfo.m_version.contains("unknown", Qt::CaseInsensitive))
        {
            QPair<int, int> versionXBounds = CalcColumnXBounds(HeaderOrder::Version);
            QRect gemVersionRect{ versionXBounds.first, contentRect.top(), versionXBounds.second - versionXBounds.first, contentRect.height() };
            painter->setFont(standardFont);
            gemVersionRect = painter->boundingRect(gemVersionRect, Qt::TextWordWrap | Qt::AlignRight | Qt::AlignVCenter, gemInfo.m_version);
            painter->drawText(gemVersionRect, Qt::TextWordWrap | Qt::AlignRight | Qt::AlignVCenter, gemInfo.m_version);

            GemSortFilterProxyModel* proxyModel = reinterpret_cast<GemSortFilterProxyModel*>(m_model);
            bool showCompatibleUpdatesOnly = proxyModel ? proxyModel->GetCompatibleFilterFlag() : true;
            if (GemModel::HasUpdates(modelIndex, showCompatibleUpdatesOnly))
            {
                painter->drawPixmap(gemVersionRect.left() - s_statusButtonSpacing - m_updatePixmap.width(),
                                    contentRect.center().y() - m_updatePixmap.height() / 2,
                                    m_updatePixmap);
            }
        }

        QRect buttonRect = CalcButtonRect(contentRect);
        DrawDownloadStatusIcon(painter, contentRect, buttonRect, modelIndex);
        if (!m_readOnly)
        {
            DrawButton(painter, buttonRect, modelIndex);
        }
        DrawPlatformText(painter, contentRect, standardFont, modelIndex);
        DrawFeatureTags(painter, contentRect, gemInfo.m_features, standardFont, summaryRect);

        painter->restore();
    }

    QRect GemItemDelegate::CalcSummaryRect(const QRect& contentRect, bool hasTags) const
    {
        const int featureTagAreaHeight = 40;
        const int summaryHeight = contentRect.height() - (hasTags * featureTagAreaHeight);

        const auto [summaryStartX, summaryEndX] = CalcColumnXBounds(HeaderOrder::Summary);

        const QSize summarySize =
            QSize(summaryEndX - summaryStartX - AdjustableHeaderWidget::s_headerTextIndent - s_extraSummarySpacing,
            summaryHeight);
        return QRect(
            QPoint(s_itemMargins.left() + summaryStartX + AdjustableHeaderWidget::s_headerTextIndent, contentRect.top()), summarySize);
    }

    QSize GemItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        int marginsHorizontal = s_itemMargins.left() + s_itemMargins.right() + s_contentMargins.left() + s_contentMargins.right();
        return QSize(marginsHorizontal + s_buttonWidth + s_defaultSummaryStartX, s_height);
    }

    bool GemItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& modelIndex)
    {
        if (!modelIndex.isValid())
        {
            return false;
        }

        if (event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<const QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Space && !m_readOnly)
            {
                const bool isAdded = GemModel::IsAdded(modelIndex);
                GemModel::SetIsAdded(*model, modelIndex, !isAdded);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            QRect fullRect, itemRect, contentRect;
            CalcRects(option, fullRect, itemRect, contentRect);

            if (!m_readOnly)
            {
                const QRect buttonRect = CalcButtonRect(contentRect);
                if (buttonRect.contains(mouseEvent->pos()))
                {
                    const bool isAdded = GemModel::IsAdded(modelIndex);
                    GemModel::SetIsAdded(*model, modelIndex, !isAdded);
                    return true;
                }
            }

            // we must manually handle html links because we aren't using QLabels
            const GemInfo& gemInfo = GemModel::GetGemInfo(modelIndex);
            const bool hasTags = !gemInfo.m_features.isEmpty();
            const QRect summaryRect = CalcSummaryRect(contentRect, hasTags);
            if (summaryRect.contains(mouseEvent->pos()))
            {
                QString anchor = anchorAt(gemInfo.m_summary, mouseEvent->pos(), summaryRect);
                if (!anchor.isEmpty())
                {
                    QDesktopServices::openUrl(QUrl(anchor));
                    return true;
                }
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, modelIndex);
    }

    QString GetGemNameList(const QVector<QPersistentModelIndex> modelIndices)
    {
        QString gemNameList;
        for (int i = 0; i < modelIndices.size(); ++i)
        {
            if (!gemNameList.isEmpty())
            {
                if (i == modelIndices.size() - 1)
                {
                    gemNameList.append(" and ");
                }
                else
                {
                    gemNameList.append(", ");
                }
            }

            gemNameList.append(GemModel::GetDisplayName(modelIndices[i]));
        }

        return gemNameList;
    }

    bool GemItemDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QRect fullRect, itemRect, contentRect;
            CalcRects(option, fullRect, itemRect, contentRect);
            const QRect buttonRect = CalcButtonRect(contentRect);
            if (buttonRect.contains(event->pos()))
            {
                if (!QToolTip::isVisible())
                {
                    if(GemModel::IsAddedDependency(index) && !GemModel::IsAdded(index))
                    {
                        const GemModel* gemModel = GemModel::GetSourceModel(index.model());
                        AZ_Assert(gemModel, "Failed to obtain GemModel");

                        // we only want to display the gems that must be de-selected to automatically
                        // disable this dependency, so don't include any that haven't been selected (added) 
                        constexpr bool addedOnly = true;
                        auto dependents = gemModel->GatherDependentGems(index, addedOnly);
                        QString nameList = GetGemNameList(dependents);
                        if (!nameList.isEmpty())
                        {
                            QToolTip::showText(event->globalPos(), tr("This gem is a dependency of %1.\nTo disable this gem, first disable %1.").arg(nameList));
                        }
                    }
                }
                return true;
            }
            else if (QToolTip::isVisible())
            {
                QToolTip::hideText();
                event->ignore();
                return true;
            }
        }

        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }

    void GemItemDelegate::CalcRects(const QStyleOptionViewItem& option, QRect& outFullRect, QRect& outItemRect, QRect& outContentRect) const
    {
        outFullRect = QRect(option.rect);
        outItemRect = QRect(outFullRect.adjusted(s_itemMargins.left(), s_itemMargins.top(), -s_itemMargins.right(), -s_itemMargins.bottom()));
        outContentRect = QRect(outItemRect.adjusted(s_contentMargins.left(), s_contentMargins.top(), -s_contentMargins.right(), -s_contentMargins.bottom()));
    }

    QRect GemItemDelegate::GetTextRect(QFont& font, const QString& text, qreal fontSize) const
    {
        font.setPixelSize(static_cast<int>(fontSize));
        return QFontMetrics(font).boundingRect(text);
    }

    QPair<int, int> GemItemDelegate::CalcColumnXBounds(HeaderOrder header) const
    {
        if (m_headerWidget)
        {
            return m_headerWidget->CalcColumnXBounds(static_cast<int>(header));
        }
        else
        {
            return QPair<int, int>(0, 0);
        }
    }

    QRect GemItemDelegate::CalcButtonRect(const QRect& contentRect) const
    {
        const QPoint topLeft = QPoint(
            s_itemMargins.left() + CalcColumnXBounds(HeaderOrder::Status).first + AdjustableHeaderWidget::s_headerTextIndent + s_statusIconSize +
                s_statusButtonSpacing,
            contentRect.center().y() - s_buttonHeight / 2);
        const QSize size = QSize(s_buttonWidth, s_buttonHeight);
        return QRect(topLeft, size);
    }

    void GemItemDelegate::DrawPlatformIcons(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const
    {
        const GemInfo::Platforms platforms = GemModel::GetGemInfo(modelIndex).m_platforms;
        int startX = s_itemMargins.left() + CalcColumnXBounds(HeaderOrder::Name).first + AdjustableHeaderWidget::s_headerTextIndent;

        // Iterate and draw the platforms in the order they are defined in the enum.
        for (int i = 0; i < GemInfo::NumPlatforms; ++i)
        {
            // Check if the platform is supported by the given gem.
            const GemInfo::Platform platform = static_cast<GemInfo::Platform>(1 << i);
            if (platforms & platform)
            {
                // Get the icon for the platform and draw it.
                const auto iterator = m_platformIcons.find(platform);
                if (iterator != m_platformIcons.end())
                {
                    const QPixmap& pixmap = iterator.value();
                    painter->drawPixmap(contentRect.left() + startX, contentRect.bottom() - s_platformIconSize, pixmap);
                    qreal aspectRatio = static_cast<qreal>(pixmap.width()) / pixmap.height();
                    startX += static_cast<int>(s_platformIconSize * aspectRatio + s_platformIconSize / 2.5);
                }
            }
        }
    }


    void GemItemDelegate::DrawPlatformText(QPainter* painter, const QRect& contentRect, const QFont& standardFont, const QModelIndex& modelIndex) const
    {
        const GemInfo::Platforms platforms = GemModel::GetGemInfo(modelIndex).m_platforms;
        
        auto xbounds = CalcColumnXBounds(HeaderOrder::Name);
        const int startX = s_platformTextleftMarginCorrection + xbounds.first;
        QFont platformFont(standardFont);
        platformFont.setPixelSize(s_featureTagFontSize);
        platformFont.setBold(false);
        painter->setFont(platformFont);
        QStringList platformList;

        //If no platforms are specified, there is nothing to draw
        if(platforms == 0)
        {
            return;
        }
        
        //UX prefers that we show platforms in reverse alphabetical order
        for(int i = GemInfo::NumPlatforms-1; i >= 0; i--)
        {
            const GemInfo::Platform platform = static_cast<GemInfo::Platform>(1 << i);
            if (platforms & platform)
            {
                platformList.append(GemInfo::GetPlatformString(platform));
            }
        }

        //figure out the ideal rect size for the platform text space constraints
        const QRect platformRect = QRect(contentRect.left() + startX, contentRect.bottom() - s_platformTextHeightAdjustment,
                                   xbounds.second -xbounds.first - s_platformTextWrapAroundMargin,
                                   (s_featureTagFontSize + s_platformTextLineBottomMargin) * s_platformTextWrapAroundLineMaxCount);

        DrawText(platformList.join(", "), painter, platformRect, platformFont);     
    }

    void GemItemDelegate::DrawFeatureTags(
        QPainter* painter,
        const QRect& contentRect,
        const QStringList& featureTags,
        const QFont& standardFont,
        const QRect& summaryRect) const
    {
        QFont gemFeatureTagFont(standardFont);
        gemFeatureTagFont.setPixelSize(s_featureTagFontSize);
        gemFeatureTagFont.setBold(false);
        painter->setFont(gemFeatureTagFont);

        int x = CalcColumnXBounds(HeaderOrder::Summary).first + AdjustableHeaderWidget::s_headerTextIndent;
        for (const QString& featureTag : featureTags)
        {
            QRect featureTagRect = GetTextRect(gemFeatureTagFont, featureTag, s_featureTagFontSize);
            featureTagRect.moveTo(s_itemMargins.left() + x + s_featureTagBorderMarginX,
                contentRect.top() + 47);
            featureTagRect = painter->boundingRect(featureTagRect, Qt::TextSingleLine, featureTag);

            QRect backgroundRect = featureTagRect;
            backgroundRect = backgroundRect.adjusted(/*left=*/-s_featureTagBorderMarginX,
                /*top=*/-s_featureTagBorderMarginY,
                /*right=*/s_featureTagBorderMarginX,
                /*bottom=*/s_featureTagBorderMarginY);

            // Skip drawing all following feature tags as there is no more space available.
            if (backgroundRect.right() > summaryRect.right())
            {
                break;
            }

            // Draw border.
            painter->setPen(m_textColor);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(backgroundRect);

            // Draw text within the border.
            painter->setPen(m_textColor);
            painter->drawText(featureTagRect, Qt::TextSingleLine, featureTag);

            x += backgroundRect.width() + s_featureTagSpacing;
        }
    }

    AZStd::unique_ptr<QTextDocument> GetTextDocument(const QString& text, int width)
    {
        // using unique_ptr as a workaround for QTextDocument having a private copy constructor
        auto doc = AZStd::make_unique<QTextDocument>();
        QTextOption textOption(doc->defaultTextOption());
        textOption.setWrapMode(QTextOption::WordWrap);
        doc->setDefaultTextOption(textOption);
        doc->setHtml(text);
        doc->setTextWidth(width);
        return doc;
    }

    void GemItemDelegate::DrawText(const QString& text, QPainter* painter, const QRect& rect, const QFont& standardFont) const 
    {
        painter->save();

        if (text.contains('<'))
        {
            painter->translate(rect.topLeft());

            // use QTextDocument because drawText does not support rich text or html
            QAbstractTextDocumentLayout::PaintContext paintContext;
            paintContext.clip = QRect(0, 0, rect.width(), rect.height());
            paintContext.palette.setColor(QPalette::Text, painter->pen().color());

            AZStd::unique_ptr<QTextDocument> textDocument = GetTextDocument(text, rect.width());
            textDocument->documentLayout()->draw(painter, paintContext);
        }
        else
        {
            painter->setFont(standardFont);
            painter->setPen(m_textColor);
            painter->drawText(rect, Qt::AlignLeft | Qt::TextWordWrap, text);
        }

        painter->restore();
    }

    void GemItemDelegate::DrawButton(QPainter* painter, const QRect& buttonRect, const QModelIndex& modelIndex) const
    {
        painter->save();
        QPoint circleCenter;

        if (GemModel::IsAdded(modelIndex))
        {
            painter->setBrush(m_buttonEnabledColor);
            painter->setPen(m_buttonEnabledColor);

            circleCenter = buttonRect.center() + QPoint(buttonRect.width() / 2 - s_buttonBorderRadius + 1, 1);
        }
        else if (GemModel::IsAddedDependency(modelIndex))
        {
            painter->setBrush(m_buttonImplicitlyEnabledColor);
            painter->setPen(m_buttonImplicitlyEnabledColor);

            circleCenter = buttonRect.center() + QPoint(buttonRect.width() / 2 - s_buttonBorderRadius + 1, 1);
        }
        else
        {
            circleCenter = buttonRect.center() + QPoint(-buttonRect.width() / 2 + s_buttonBorderRadius + 1, 1);
        }

        // Rounded rect
        painter->drawRoundedRect(buttonRect, s_buttonBorderRadius, s_buttonBorderRadius);

        // Circle
        painter->setBrush(m_textColor);
        painter->drawEllipse(circleCenter, s_buttonCircleRadius, s_buttonCircleRadius);

        painter->restore();
    }

    QString GemItemDelegate::anchorAt(const QString& html, const QPoint& position, const QRect& rect)
    {
        if (!html.isEmpty())
        {
            AZStd::unique_ptr<QTextDocument> doc = GetTextDocument(html, rect.width());
            QAbstractTextDocumentLayout* layout = doc->documentLayout();
            if (layout)
            {
                return layout->anchorAt(position - rect.topLeft());
            }
        }

        return QString();
    }

    void GemItemDelegate::DrawDownloadStatusIcon(QPainter* painter, const QRect& contentRect, const QRect& buttonRect, const QModelIndex& modelIndex) const
    {
        if(GemModel::GetGemInfo(modelIndex).m_gemOrigin != GemInfo::Remote)
        {
            return;
        }

        const GemInfo::DownloadStatus downloadStatus = GemModel::GetDownloadStatus(modelIndex);

        QPixmap currentFrame;
        const QPixmap* statusPixmap;
        if (downloadStatus == GemInfo::DownloadStatus::Downloaded)
        {
            statusPixmap = &m_downloadedPixmap;
        }
        else if (downloadStatus == GemInfo::DownloadStatus::Downloading)
        {
            if (m_downloadingMovie->state() != QMovie::Running)
            {
                m_downloadingMovie->start();
                emit MovieStartedPlaying(m_downloadingMovie);
            }

            currentFrame = m_downloadingMovie->currentPixmap();
            currentFrame = currentFrame.scaled(s_statusIconSize, s_statusIconSize);
            statusPixmap = &currentFrame;
        }
        else if (downloadStatus == GemInfo::DownloadStatus::DownloadSuccessful)
        {
            statusPixmap = &m_downloadSuccessfulPixmap;
        }
        else if (downloadStatus == GemInfo::DownloadStatus::DownloadFailed)
        {
            statusPixmap = &m_downloadFailedPixmap;
        }
        else if (downloadStatus == GemInfo::DownloadStatus::NotDownloaded)
        {
            statusPixmap = &m_notDownloadedPixmap;
        }
        else
        {
            statusPixmap = &m_unknownStatusPixmap;
        }

        QSize statusSize = statusPixmap->size();

        if (m_readOnly)
        {
            // for now, we don't draw the status button in read only state
            // so draw the status icon centered
            painter->drawPixmap(
                buttonRect.center().x() - statusSize.width() / 2,
                contentRect.center().y() - statusSize.height() / 2,
                *statusPixmap);
        }
        else
        {
            painter->drawPixmap(
                buttonRect.left() - s_statusButtonSpacing - statusSize.width(),
                contentRect.center().y() - statusSize.height() / 2,
                *statusPixmap);
        }
    }
} // namespace O3DE::ProjectManager

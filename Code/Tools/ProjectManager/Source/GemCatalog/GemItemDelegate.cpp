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

namespace O3DE::ProjectManager
{
    GemItemDelegate::GemItemDelegate(QAbstractItemModel* model, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_model(model)
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
        float aspectRatio = static_cast<float>(pixmap.width()) / pixmap.height();
        int xScaler = s_statusIconSize;
        int yScaler = s_statusIconSize;

        if (aspectRatio > 1.0f)
        {
            yScaler = static_cast<int>(1.0f / aspectRatio * s_statusIconSize);
        }
        else if (aspectRatio < 1.0f)
        {
            xScaler = static_cast<int>(aspectRatio * s_statusIconSize);
        }

        m_iconPixmap = QPixmap(QIcon(iconPath).pixmap(xScaler, yScaler));
    }

    void GemItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid())
        {
            return;
        }

        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        painter->setRenderHint(QPainter::Antialiasing);

        QRect fullRect, itemRect, contentRect;
        CalcRects(options, fullRect, itemRect, contentRect);

        QRect buttonRect = CalcButtonRect(contentRect);

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

        // Gem name
        QString gemName = GemModel::GetDisplayName(modelIndex);
        QFont gemNameFont(options.font);
        const int firstColumnMaxTextWidth = s_summaryStartX - 30;
        gemNameFont.setPixelSize(static_cast<int>(s_gemNameFontSize));
        gemNameFont.setBold(true);
        gemName = QFontMetrics(gemNameFont).elidedText(gemName, Qt::TextElideMode::ElideRight, firstColumnMaxTextWidth);
        QRect gemNameRect = GetTextRect(gemNameFont, gemName, s_gemNameFontSize);
        gemNameRect.moveTo(contentRect.left(), contentRect.top());
        painter->setFont(gemNameFont);
        painter->setPen(m_textColor);
        gemNameRect = painter->boundingRect(gemNameRect, Qt::TextSingleLine, gemName);
        painter->drawText(gemNameRect, Qt::TextSingleLine, gemName);

        // Gem creator
        QString gemCreator = GemModel::GetCreator(modelIndex);
        gemCreator = standardFontMetrics.elidedText(gemCreator, Qt::TextElideMode::ElideRight, firstColumnMaxTextWidth);
        QRect gemCreatorRect = GetTextRect(standardFont, gemCreator, s_fontSize);
        gemCreatorRect.moveTo(contentRect.left(), contentRect.top() + gemNameRect.height());

        painter->setFont(standardFont);
        gemCreatorRect = painter->boundingRect(gemCreatorRect, Qt::TextSingleLine, gemCreator);
        painter->drawText(gemCreatorRect, Qt::TextSingleLine, gemCreator);

        // Gem summary
        const QStringList featureTags = GemModel::GetFeatures(modelIndex);
        const bool hasTags = !featureTags.isEmpty();
        const QString summary = GemModel::GetSummary(modelIndex);
        const QRect summaryRect = CalcSummaryRect(contentRect, hasTags);
        DrawText(summary, painter, summaryRect, standardFont);

        DrawDownloadStatusIcon(painter, contentRect, buttonRect, modelIndex);
        DrawButton(painter, buttonRect, modelIndex);
        DrawPlatformIcons(painter, contentRect, modelIndex);
        DrawFeatureTags(painter, contentRect, featureTags, standardFont, summaryRect);

        painter->restore();
    }

    QRect GemItemDelegate::CalcSummaryRect(const QRect& contentRect, bool hasTags) const
    {
        const int featureTagAreaHeight = 30;
        const int summaryHeight = contentRect.height() - (hasTags * featureTagAreaHeight);

        const int additionalSummarySpacing = s_itemMargins.right() * 3;
        const QSize summarySize = QSize(contentRect.width() - s_summaryStartX - s_buttonWidth - additionalSummarySpacing,
            summaryHeight);
        return QRect(QPoint(contentRect.left() + s_summaryStartX, contentRect.top()), summarySize);
    }

    QSize GemItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& modelIndex) const
    {
        QStyleOptionViewItem options(option);
        initStyleOption(&options, modelIndex);

        int marginsHorizontal = s_itemMargins.left() + s_itemMargins.right() + s_contentMargins.left() + s_contentMargins.right();
        return QSize(marginsHorizontal + s_buttonWidth + s_summaryStartX, s_height);
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
            if (keyEvent->key() == Qt::Key_Space)
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
            const QRect buttonRect = CalcButtonRect(contentRect);

            if (buttonRect.contains(mouseEvent->pos()))
            {
                const bool isAdded = GemModel::IsAdded(modelIndex);
                GemModel::SetIsAdded(*model, modelIndex, !isAdded);
                return true;
            }

            // we must manually handle html links because we aren't using QLabels
            const QStringList featureTags = GemModel::GetFeatures(modelIndex);
            const bool hasTags = !featureTags.isEmpty();
            const QRect summaryRect = CalcSummaryRect(contentRect, hasTags);
            if (summaryRect.contains(mouseEvent->pos()))
            {
                const QString html = GemModel::GetSummary(modelIndex);
                QString anchor = anchorAt(html, mouseEvent->pos(), summaryRect);
                if (!anchor.isEmpty())
                {
                    QDesktopServices::openUrl(QUrl(anchor));
                    return true;
                }
            }
        }

        return QStyledItemDelegate::editorEvent(event, model, option, modelIndex);
    }

    QString GetGemNameList(const QVector<QModelIndex> modelIndices)
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
                        QVector<QModelIndex> dependents = gemModel->GatherDependentGems(index, addedOnly);
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

    QRect GemItemDelegate::CalcButtonRect(const QRect& contentRect) const
    {
        const QPoint topLeft = QPoint(contentRect.right() - s_buttonWidth, contentRect.center().y() - s_buttonHeight / 2);
        const QSize size = QSize(s_buttonWidth, s_buttonHeight);
        return QRect(topLeft, size);
    }

    void GemItemDelegate::DrawPlatformIcons(QPainter* painter, const QRect& contentRect, const QModelIndex& modelIndex) const
    {
        const GemInfo::Platforms platforms = GemModel::GetPlatforms(modelIndex);
        int startX = 0;

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

    void GemItemDelegate::DrawFeatureTags(QPainter* painter, const QRect& contentRect, const QStringList& featureTags, const QFont& standardFont, const QRect& summaryRect) const
    {
        QFont gemFeatureTagFont(standardFont);
        gemFeatureTagFont.setPixelSize(s_featureTagFontSize);
        gemFeatureTagFont.setBold(false);
        painter->setFont(gemFeatureTagFont);

        int x = s_summaryStartX;
        for (const QString& featureTag : featureTags)
        {
            QRect featureTagRect = GetTextRect(gemFeatureTagFont, featureTag, s_featureTagFontSize);
            featureTagRect.moveTo(contentRect.left() + x + s_featureTagBorderMarginX,
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
            circleCenter = buttonRect.center() + QPoint(-buttonRect.width() / 2 + s_buttonBorderRadius, 1);
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
        const GemInfo::DownloadStatus downloadStatus = GemModel::GetDownloadStatus(modelIndex);

        // Show no icon if gem is already downloaded
        if (downloadStatus == GemInfo::DownloadStatus::Downloaded)
        {
            return;
        }

        QPixmap currentFrame;
        const QPixmap* statusPixmap;
        if (downloadStatus == GemInfo::DownloadStatus::Downloading)
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

        painter->drawPixmap(
            buttonRect.left() - s_statusButtonSpacing - statusSize.width(),
            contentRect.center().y() - statusSize.height() / 2,
            *statusPixmap);
    }
} // namespace O3DE::ProjectManager

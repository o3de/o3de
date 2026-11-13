/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LinkWidget.h>
#include <ExternalLinkDialog.h>
#include <FlatElements.h>
#include <FlatValues.h>
#include <SettingsInterface.h>

#include <AzCore/Debug/Trace.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QDesktopServices>
#include <QEvent>
#include <QMouseEvent>
#include <QStyle>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    LinkLabel::LinkLabel(const QString& text, const QUrl& url, int fontSize, QWidget* parent)
        : AzQtComponents::ElidingLabel(text, parent)
        , m_url(url)
        , m_fontSize(fontSize)
    {
        // The flattened GUI draws the sizes FlatElements.cpp has a text element for; any other would show there as a plain label
        AZ_Warning("ProjectManager", !AzQtComponents::StyleManager::stylesheetsDisabled() || HasFlatLinkLabelText(fontSize),
            "A LinkLabel of %d px has no text element in the flattened GUI (FlatElements.cpp): it shows as a plain label there.",
            fontSize);
        SetDefaultStyle();
    }

    void LinkLabel::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        if (m_url.isValid())
        {
            // Check if user request not to be shown external link warning dialog
            bool skipDialog = false;
            SettingsInterface::Get()->Get(skipDialog, ISettings::ExternalLinkWarningKey);

            if (!skipDialog)
            {
                // Style does not apply if LinkLabel is parent so use parentWidget as parent instead
                ExternalLinkDialog* linkDialog = new ExternalLinkDialog(m_url, parentWidget());
                if (linkDialog->exec() == QDialog::Accepted)
                {
                    QDesktopServices::openUrl(m_url);
                }
            }
            else
            {
                QDesktopServices::openUrl(m_url);
            }
        }

        emit clicked();
    }

    void LinkLabel::enterEvent([[maybe_unused]] QEnterEvent* event)
    {
        // The legacy GUI's hover sheet, with the underline. The flattened GUI sets none: it marks the state, and its text elements
        // (FlatElements.cpp) carry the underline with the size and the colour.
        AzQtComponents::StyleManager::setLegacyStyleSheet(
            this, QString("font-size: %1px; color: %2; text-decoration: underline;")
                .arg(m_fontSize).arg(FlatValues::LinkLabelColor.name().toUpper()));
        SetHoverState(true);
    }

    void LinkLabel::leaveEvent([[maybe_unused]] QEvent* event)
    {
        SetDefaultStyle();
        SetHoverState(false);
    }

    QUrl LinkLabel::GetUrl() const
    {
        return m_url;
    }

    void LinkLabel::SetUrl(const QUrl& url)
    {
        m_url = url;
    }

    int LinkLabel::GetFontSize() const
    {
        return m_fontSize;
    }

    void LinkLabel::SetDefaultStyle()
    {
        AzQtComponents::StyleManager::setLegacyStyleSheet(this,
            QString("font-size: %1px; color: %2;").arg(m_fontSize).arg(FlatValues::LinkLabelColor.name().toUpper()));
    }

    void LinkLabel::SetHoverState(bool hovered)
    {
        if (!AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            return;
        }

        // The property is what the matchers of the hover elements read, and the polish resolves the element of the state, as the
        // change of the sheet did in the legacy GUI: for the link, and for the label it keeps inside to measure its text, which the
        // sheet reached as it reaches every widget inside
        setProperty(HoverProperty, hovered);
        RefreshStyle();
        for (QLabel* inside : findChildren<QLabel*>(Qt::FindDirectChildrenOnly))
        {
            inside->style()->unpolish(inside);
            inside->style()->polish(inside);
        }
    }
} // namespace O3DE::ProjectManager

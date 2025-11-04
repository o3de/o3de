/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "MainStatusBar.h"

#include <AzCore/Utils/Utils.h>
#include <AzFramework/Asset/AssetSystemBus.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>

// Qt
#include <QMenu>
#include <QStyleOption>
#include <QStylePainter>
#include <QTimer>
#include <QCheckBox>
#include <QWidgetAction>

// Editor
#include "MainStatusBarItems.h"
#include "ProcessInfo.h"


const int iconTextSpacing = 3;
const int marginSpacing = 2;
const int spacerSpacing = 5;
const int spacerColor = 0x6F6D6D;

StatusBarItem::StatusBarItem(const QString& name, bool isClickable, MainStatusBar* parent, bool hasLeadingSpacer)
    : QWidget(parent)
    , m_isClickable(isClickable)
    , m_hasLeadingSpacer(hasLeadingSpacer)
{
    setObjectName(name);
}

StatusBarItem::StatusBarItem(const QString& name, MainStatusBar* parent, bool hasLeadingSpacer)
    : StatusBarItem(name, false, parent, hasLeadingSpacer)
{}

void StatusBarItem::SetText(const QString& text)
{
    if (text != m_text)
    {
        m_text = text;
        updateGeometry();
        update();
    }
}

void StatusBarItem::SetIcon(const QPixmap& icon)
{
    QIcon origIcon = m_icon;

    if (icon.isNull())
    {
        m_icon = icon;
    }
    else
    {
        // avoid generating new pixmaps if we don't need to.
        if (icon.height() != 16)
        {
            m_icon = icon.scaledToHeight(16);
        }
        else
        {
            m_icon = icon;
        }
    }

    if (icon.isNull() ^ origIcon.isNull())
    {
        updateGeometry();
    }

    // don't generate paintevents unless we absolutely have changed!
    if (origIcon.cacheKey() != m_icon.cacheKey())
    {
        update();
    }
}

void StatusBarItem::SetIcon(const QIcon& icon)
{
    QIcon origIcon = m_icon;

    m_icon = icon;

    if (icon.isNull() ^ origIcon.isNull())
    {
        updateGeometry();
    }

    // don't generate paintevents unless we absolutely have changed!
    if (origIcon.cacheKey() != m_icon.cacheKey())
    {
        update();
    }
}

void StatusBarItem::SetToolTip(const QString& tip)
{
    setToolTip(tip);
}

void StatusBarItem::mousePressEvent(QMouseEvent* e)
{
    if (m_isClickable && e->button() == Qt::LeftButton)
    {
        emit clicked();
    }
}

QSize StatusBarItem::sizeHint() const
{
    QSize hint(4, 20);
    if (!m_icon.isNull())
    {
        hint.rwidth() += 16;
    }
    if (!m_icon.isNull() && !CurrentText().isEmpty())
    {
        hint.rwidth() += iconTextSpacing; //spacing
    }
    auto fm = fontMetrics();
    hint.rwidth() += fm.horizontalAdvance(CurrentText());

    hint.rwidth() += 2 * marginSpacing;
    hint.rheight() += 2 * marginSpacing;

    hint.rwidth() += m_hasLeadingSpacer ? spacerSpacing : 0;

    return hint;
}

QSize StatusBarItem::minimumSizeHint() const
{
    return sizeHint();
}

void StatusBarItem::paintEvent([[maybe_unused]] QPaintEvent* pe)
{
    QStylePainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    painter.drawPrimitive(QStyle::PE_Widget, opt);

    auto rect = contentsRect();
    rect.adjust(marginSpacing, marginSpacing, -marginSpacing, -marginSpacing);

    QRect textRect = rect;
    if (m_hasLeadingSpacer)
    {
        textRect.adjust(spacerSpacing, 0, 0, 0);

    }
    if (!CurrentText().isEmpty())
    {
        painter.drawItemText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->palette(), true, CurrentText(), QPalette::WindowText);
    }

    if (!m_icon.isNull())
    {
        auto textWidth = textRect.width();
        if (textWidth > 0)
        {
            textWidth += iconTextSpacing; //margin
        }
        QRect iconRect = { textRect.left() + textWidth - textRect.height() - 1, textRect.top() + 2,  textRect.height() - 4, textRect.height() - 4 };
        m_icon.paint(&painter, iconRect, Qt::AlignCenter);
    }

    if (m_hasLeadingSpacer)
    {
        QPen pen{ spacerColor };
        painter.setPen(pen);
        painter.drawLine(spacerSpacing / 2, 3, spacerSpacing / 2, rect.height() + 2);
    }
}

QString StatusBarItem::CurrentText() const
{
    return m_text;
}

MainStatusBar* StatusBarItem::StatusBar() const
{
    return static_cast<MainStatusBar*>(parentWidget());
}

///////////////////////////////////////////////////////////////////////////////////////

MainStatusBar::MainStatusBar(QWidget* parent)
    : QStatusBar(parent)
{
    addPermanentWidget(new GeneralStatusItem(QStringLiteral("status"), this), 50);

    addPermanentWidget(new SourceControlItem(QStringLiteral("source_control"), this), 1);

    addPermanentWidget(new StatusBarItem(QStringLiteral("connection"), true, this, true), 1);

    addPermanentWidget(new GameInfoItem(QStringLiteral("game_info"), this), 1);

    addPermanentWidget(new MemoryStatusItem(QStringLiteral("memory"), this), 1);
}

void MainStatusBar::Init()
{
    //called on mainwindow initialization
    const int statusbarTimerUpdateInterval{
        500
    };                                             //in ms, so 2 FPS

    //ask for updates for items regularly. This is basically what MFC does
    auto timer = new QTimer(this);
    timer->setInterval(statusbarTimerUpdateInterval);
    connect(timer, &QTimer::timeout, this, &MainStatusBar::requestStatusUpdate);
    timer->start();
}

void MainStatusBar::SetStatusText(const QString& text)
{
    SetItem(QStringLiteral("status"), text, QString(), QPixmap());
}

QWidget* MainStatusBar::SetItem(QString indicatorName, QString text, QString tip, const QPixmap& icon)
{
    auto item = findChild<StatusBarItem*>(indicatorName, Qt::FindDirectChildrenOnly);
    assert(item);

    item->SetText(text);
    item->SetToolTip(tip);
    item->SetIcon(icon);

    return item;
}

QWidget* MainStatusBar::SetItem(QString indicatorName, QString text, QString tip, int iconId)
{
    static std::unordered_map<int, QPixmap> idImages {
        {
            IDI_BALL_DISABLED, QPixmap(QStringLiteral(":/statusbar/res/ball_disabled.ico")).scaledToHeight(16)
        },
        {
            IDI_BALL_OFFLINE, QPixmap(QStringLiteral(":/statusbar/res/ball_offline.ico")).scaledToHeight(16)
        },
        {
            IDI_BALL_ONLINE, QPixmap(QStringLiteral(":/statusbar/res/ball_online.ico")).scaledToHeight(16)
        },
        {
            IDI_BALL_PENDING, QPixmap(QStringLiteral(":/statusbar/res/ball_pending.ico")).scaledToHeight(16)
        }
    };

    auto search = idImages.find(iconId);

    return SetItem(indicatorName, text, tip, search == idImages.end() ? QPixmap() : search->second);
}


QWidget* MainStatusBar::GetItem(QString indicatorName)
{
    return findChild<QWidget*>(indicatorName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

SourceControlItem::SourceControlItem(QString name, MainStatusBar* parent)
    : StatusBarItem(name, true, parent)
    , m_sourceControlAvailable(false)
{
    if (AzToolsFramework::SourceControlConnectionRequestBus::HasHandlers())
    {
        AzToolsFramework::SourceControlNotificationBus::Handler::BusConnect();
        m_sourceControlAvailable = true;
    }

    InitMenu();
    connect(this, &StatusBarItem::clicked, this, &SourceControlItem::UpdateAndShowMenu);
}

SourceControlItem::~SourceControlItem()
{
    AzToolsFramework::SourceControlNotificationBus::Handler::BusDisconnect();
}

void SourceControlItem::UpdateAndShowMenu()
{
    if (m_sourceControlAvailable)
    {
        UpdateMenuItems();
        m_menu->popup(QCursor::pos());
    }
}

void SourceControlItem::ConnectivityStateChanged(const AzToolsFramework::SourceControlState state)
{
    AzToolsFramework::SourceControlState oldState = m_SourceControlState;
    m_SourceControlState = state;
    UpdateMenuItems();

    if (oldState != m_SourceControlState)
    {
        // if the user has turned the system on or off, signal the asset processor
        // we know the user has turned the system off if the old state was disabled
        // or if the new state is disabled (when the state changed)
        if ((oldState == AzToolsFramework::SourceControlState::Disabled) || (m_SourceControlState == AzToolsFramework::SourceControlState::Disabled))
        {
            bool enabled = m_SourceControlState != AzToolsFramework::SourceControlState::Disabled;
            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::UpdateSourceControlStatus, enabled);
        }
    }
}

void SourceControlItem::InitMenu()
{
    m_scIconOk = QIcon(":statusbar/res/source_control_connected.svg");
    m_scIconError = QIcon(":statusbar/res/source_control_error_v2.svg");
    m_scIconWarning = QIcon(":statusbar/res/source_control-warning_v2.svg");
    m_scIconDisabled = QIcon(":statusbar/res/source_control-not_setup.svg");

    if (m_sourceControlAvailable)
    {
        m_menu = std::make_unique<QMenu>();

        m_settingsAction = m_menu->addAction(tr("Settings"));
        m_checkBox = new QCheckBox(m_menu.get());
        m_checkBox->setText(tr("Enable"));
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_checkBox);
        m_enableAction = new QWidgetAction(m_menu.get());
        m_enableAction->setDefaultWidget(m_checkBox);
        m_menu->addAction(m_settingsAction);
        m_menu->addAction(m_enableAction);
        AzQtComponents::Style::addClass(m_menu.get(), "SourceControlMenu");
        m_enableAction->setCheckable(true);

        m_enableAction->setEnabled(true);

        {
            using namespace AzToolsFramework;
            m_SourceControlState = SourceControlState::Disabled;
            SourceControlConnectionRequestBus::BroadcastResult(m_SourceControlState, &SourceControlConnectionRequestBus::Events::GetSourceControlState);
            UpdateMenuItems();
        }

        connect(m_settingsAction, &QAction::triggered, this, &SourceControlItem::OnOpenSettings);
        connect(m_checkBox, &QCheckBox::stateChanged, this, [this](int state) {SetSourceControlEnabledState(state); });
    }
    else
    {
        SetIcon(m_scIconDisabled);
        SetToolTip(tr("No source control provided"));
    }
    SetText("P4V");
}
void SourceControlItem::OnOpenSettings() {

    // LEGACY note - GetIEditor and GetSourceControl are both legacy functions
    // but are currently the only way to actually show the source control settings.
    // They come from an editor plugin which should be moved to a gem eventually instead.
    // For now, the actual function of the p4 plugin (so for example, checking file status, checking out files, etc)
    // lives in AzToolsFramework, and is always available,
    // but the settings dialog lives in the plugin, which is not always available, on all platforms, and thus
    // this pointer could be null despite P4 still functioning.  (For example, it can function via command line on linux,
    // and its settings can come from the P4 ENV or P4 Client env, without having to show the GUI).  Therefore
    // it is still valid to have m_sourceControlAvailable be true yet GetIEditor()->GetSourceControl() be null.
    using namespace AzToolsFramework;
    SourceControlConnectionRequestBus::Broadcast(&SourceControlConnectionRequestBus::Events::OpenSettings);

}
void SourceControlItem::SetSourceControlEnabledState(bool state)
{
    using SCRequest = AzToolsFramework::SourceControlConnectionRequestBus;
    SCRequest::Broadcast(&SCRequest::Events::EnableSourceControl, state);
    m_menu->hide();
}

void SourceControlItem::UpdateMenuItems()
{
    QString toolTip;
    bool disabled = false;
    bool errorIcon = false;
    bool invalidConfig = false;

    switch (m_SourceControlState)
    {
    case AzToolsFramework::SourceControlState::Disabled:
        toolTip = tr("Perforce disabled");
        disabled = true;
        errorIcon = true;
        break;
    case AzToolsFramework::SourceControlState::ConfigurationInvalid:
        errorIcon = true;
        invalidConfig = true;
        toolTip = tr("Perforce configuration invalid");
        break;
    case AzToolsFramework::SourceControlState::Active:
        toolTip = tr("Perforce connected");
        break;
    }

    m_settingsAction->setEnabled(!disabled);
    m_checkBox->setChecked(!disabled);
    m_checkBox->setText(disabled ? tr("Status: Offline") : invalidConfig ? tr("Status: Invalid Configuration - check the console log") : tr("Status: Online"));
    SetIcon(errorIcon ? disabled ? m_scIconDisabled : m_scIconWarning : m_scIconOk);
    SetToolTip(toolTip);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

MemoryStatusItem::MemoryStatusItem(QString name, MainStatusBar* parent)
    : StatusBarItem(name, parent)
{
    connect(parent, &MainStatusBar::requestStatusUpdate, this, &MemoryStatusItem::updateStatus);
    SetToolTip(tr("Memory usage"));
}

MemoryStatusItem::~MemoryStatusItem()
{
}

void MemoryStatusItem::updateStatus()
{
    AZ::ProcessMemInfo mi;
    AZ::QueryMemInfo(mi);

    uint64 nSizeMb = (uint64)(mi.m_workingSet / (1024 * 1024));

    SetText(QString("%1 Mb").arg(nSizeMb));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralStatusItem::GeneralStatusItem(QString name, MainStatusBar* parent)
    : StatusBarItem(name, parent)
{
    connect(parent, &MainStatusBar::messageChanged, this, [this](const QString&) { update(); });
}

QString GeneralStatusItem::CurrentText() const
{
    if (!StatusBar()->currentMessage().isEmpty())
    {
        return StatusBar()->currentMessage();
    }

    return StatusBarItem::CurrentText();
}

GameInfoItem::GameInfoItem(QString name, MainStatusBar* parent)
    : StatusBarItem(name, parent, true)
{
    m_projectPath = QString::fromUtf8(AZ::Utils::GetProjectPath().c_str());

    SetText(QObject::tr("GameFolder: '%1'").arg(m_projectPath));
    SetToolTip(QObject::tr("Game Info"));

    setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, &QWidget::customContextMenuRequested, this, &GameInfoItem::OnShowContextMenu);
}

void GameInfoItem::OnShowContextMenu(const QPoint& pos)
{
    QMenu contextMenu(this);

    // Context menu action to open the project folder in file browser
    contextMenu.addAction(AzQtComponents::fileBrowserActionName(), this, [this]() {
        AzQtComponents::ShowFileOnDesktop(m_projectPath);
    });

    contextMenu.exec(mapToGlobal(pos));
}

#include <moc_MainStatusBar.cpp>
#include <moc_MainStatusBarItems.cpp>

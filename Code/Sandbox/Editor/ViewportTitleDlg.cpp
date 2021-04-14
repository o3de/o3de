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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : CViewportTitleDlg implementation file


#include "EditorDefs.h"

#include "ViewportTitleDlg.h"

// Qt
#include <QInputDialog>

// CryCommon
#include <CryCommon/SFunctor.h>

// Editor
#include "Settings.h"
#include "ViewPane.h"
#include "DisplaySettings.h"
#include "CustomResolutionDlg.h"
#include "CustomAspectRatioDlg.h"
#include "Include/IObjectManager.h"
#include "UsedResources.h"
#include "Objects/SelectionGroup.h"
#include "UsedResources.h"
#include "Include/IObjectManager.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_ViewportTitleDlg.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


// CViewportTitleDlg dialog

inline namespace Helpers
{
    void ToggleHelpers()
    {
        GetIEditor()->GetDisplaySettings()->DisplayHelpers(!GetIEditor()->GetDisplaySettings()->IsDisplayHelpers());
        GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);

        if (GetIEditor()->GetDisplaySettings()->IsDisplayHelpers() == false)
        {
            GetIEditor()->GetObjectManager()->SendEvent(EVENT_HIDE_HELPER);
        }
    }

    bool IsHelpersShown()
    {
        return GetIEditor()->GetDisplaySettings()->IsDisplayHelpers();
    }
}

const int SEARCH_BY_NAME = 1;
const int SEARCH_BY_TYPE = 2;
const int SEARCH_BY_ASSET = 3;
const int HIDE_OTHERS = 4;
const int FREEZE_OTHERS = 5;
const int JUST_SELLECT = 6;
const int LOGICAL_AND = 7;
const int LOGICAL_OR = 8;
const int INPUT_NAMES_FROM_SELECTION = 9;

CViewportTitleDlg::CViewportTitleDlg(QWidget* pParent)
    : QWidget(pParent)
    , m_searchMode(ESM_BY_NAME)
    , m_searchResultHandling(ESRH_HIDE_OTHERS)
    , m_bOR(false)
    , m_ui(new Ui::ViewportTitleDlg)
    , m_searchModeActionGroup(nullptr)
    , m_searchByNameAction(nullptr)
    , m_searchByTypeAction(nullptr)
    , m_searchByAssetAction(nullptr)
    , m_searchResultHandlingActionGroup(nullptr)
    , m_searchHideOthersAction(nullptr)
    , m_searchFreezeOthersAction(nullptr)
    , m_searchJustSelectAction(nullptr)
    , m_searchMatchTypeActionGroup(nullptr)
    , m_searchAndAction(nullptr)
    , m_searchOrAction(nullptr)
{
    auto container = new QWidget(this);
    m_ui->setupUi(container);
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(container);
    container->setObjectName("ViewportTitleDlgContainer");

    m_pViewPane = NULL;
    GetIEditor()->RegisterNotifyListener(this);
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);

    LoadCustomPresets("FOVPresets", "FOVPreset", m_customFOVPresets);
    LoadCustomPresets("AspectRatioPresets", "AspectRatioPreset", m_customAspectRatioPresets);
    LoadCustomPresets("ResPresets", "ResPreset", m_customResPresets);

    const bool newViewportInteractionModelEnabled =
        GetIEditor()->IsNewViewportInteractionModelEnabled();

    m_ui->m_viewportSearch->setEnabled(!newViewportInteractionModelEnabled);
    m_ui->m_viewportSearch->setVisible(!newViewportInteractionModelEnabled);

    OnInitDialog();

    if (!newViewportInteractionModelEnabled)
    {
        m_ui->m_viewportSearch->setClearButtonEnabled(true);
        connect(m_ui->m_viewportSearch, &AzQtComponents::SearchLineEdit::menuEntryClicked, this, &CViewportTitleDlg::OnViewportSearchButtonClicked);
        connect(m_ui->m_viewportSearch, &AzQtComponents::SearchLineEdit::returnPressed, this, &CViewportTitleDlg::OnSearchTermChange);
        connect(m_ui->m_viewportSearch, &QLineEdit::textChanged, this, [this] (const QString& text){
            if(text.isEmpty())
            {
                this->OnViewportSearchClear();
            }
        });

        m_ui->m_viewportSearch->setFixedWidth(190);
    }

    connect(m_ui->m_fovLabel, &QWidget::customContextMenuRequested, this, &CViewportTitleDlg::PopUpFOVMenu);
    connect(m_ui->m_fovStaticCtrl, &QWidget::customContextMenuRequested, this, &CViewportTitleDlg::PopUpFOVMenu);
    connect(m_ui->m_ratioStaticCtrl, &QWidget::customContextMenuRequested, this, &CViewportTitleDlg::PopUpAspectMenu);
    connect(m_ui->m_ratioLabel, &QWidget::customContextMenuRequested, this, &CViewportTitleDlg::PopUpAspectMenu);
    connect(m_ui->m_sizeStaticCtrl, &QWidget::customContextMenuRequested, this, &CViewportTitleDlg::PopUpResolutionMenu);
}

CViewportTitleDlg::~CViewportTitleDlg()
{
    GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
    ICVar*  pDisplayInfo(gEnv->pConsole->GetCVar("r_displayInfo"));
    pDisplayInfo->RemoveOnChangeFunctor(m_displayInfoCallbackIndex);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetViewPane(CLayoutViewPane* pViewPane)
{
    if (m_pViewPane)
        m_pViewPane->disconnect(this);
    m_pViewPane = pViewPane;
    if (m_pViewPane)
        connect(this, &QWidget::customContextMenuRequested, m_pViewPane, &CLayoutViewPane::ShowTitleMenu);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnInitDialog()
{
    m_ui->m_titleBtn->setText(m_title);
    m_ui->m_sizeStaticCtrl->setText(QString());

    UpdateSearchOptionsText();

    m_ui->m_toggleHelpersBtn->setChecked(GetIEditor()->GetDisplaySettings()->IsDisplayHelpers());

    ICVar*  pDisplayInfo(gEnv->pConsole->GetCVar("r_displayInfo"));
    if (pDisplayInfo)
    {
        SFunctor oFunctor;
        oFunctor.Set(OnChangedDisplayInfo, pDisplayInfo, m_ui->m_toggleDisplayInfoBtn);
        m_displayInfoCallbackIndex = pDisplayInfo->AddOnChangeFunctor(oFunctor);
        OnChangedDisplayInfo(pDisplayInfo, m_ui->m_toggleDisplayInfoBtn);
    }

    connect(m_ui->m_toggleHelpersBtn, &QToolButton::clicked, this, &CViewportTitleDlg::OnToggleHelpers);
    connect(m_ui->m_toggleDisplayInfoBtn, &QToolButton::clicked, this, &CViewportTitleDlg::OnToggleDisplayInfo);

    m_ui->m_toggleHelpersBtn->setProperty("class", "big");
    m_ui->m_toggleDisplayInfoBtn->setProperty("class", "big");

    //  ON_EN_CHANGE(IDC_VIEWPORT_SEARCH, OnSearchTermChange)
}

//////////////////////////////////////////////////////////////////////////
QMenu * CViewportTitleDlg::InitializeViewportSearchMenu()
{
    QMenu* menu = new QMenu(this);

    m_searchByNameAction = menu->addAction(tr("By Name"));
    m_searchByNameAction->setData(SEARCH_BY_NAME);
    m_searchByNameAction->setCheckable(true);
    m_searchByNameAction->setChecked(m_searchMode == ESM_BY_NAME);
    m_searchByTypeAction = menu->addAction(tr("By Type"));
    m_searchByTypeAction->setData(SEARCH_BY_TYPE);
    m_searchByTypeAction->setCheckable(true);
    m_searchByTypeAction->setChecked(m_searchMode == ESM_BY_TYPE);
    m_searchByAssetAction = menu->addAction(tr("By Asset"));
    m_searchByAssetAction->setData(SEARCH_BY_ASSET);
    m_searchByAssetAction->setCheckable(true);
    m_searchByAssetAction->setChecked(m_searchMode == ESM_BY_ASSET);

    menu->addSeparator();

    m_searchHideOthersAction = menu->addAction(tr("Hide others"));
    m_searchHideOthersAction->setData(HIDE_OTHERS);
    m_searchHideOthersAction->setCheckable(true);
    m_searchHideOthersAction->setChecked(m_searchResultHandling == ESRH_HIDE_OTHERS);
    m_searchFreezeOthersAction = menu->addAction(tr("Freeze others"));
    m_searchFreezeOthersAction->setData(FREEZE_OTHERS);
    m_searchFreezeOthersAction->setCheckable(true);
    m_searchFreezeOthersAction->setChecked(m_searchResultHandling == ESRH_FREEZE_OTHERS);
    m_searchJustSelectAction = menu->addAction(tr("Just Select"));
    m_searchJustSelectAction->setData(JUST_SELLECT);
    m_searchJustSelectAction->setCheckable(true);
    m_searchJustSelectAction->setChecked(m_searchResultHandling == ESRH_JUST_SELECT);

    menu->addSeparator();

    m_searchAndAction = menu->addAction(tr("AND"));
    m_searchAndAction->setData(LOGICAL_AND);
    m_searchAndAction->setCheckable(true);
    m_searchAndAction->setChecked(!m_bOR);
    m_searchOrAction = menu->addAction(tr("OR"));
    m_searchOrAction->setData(LOGICAL_OR);
    m_searchOrAction->setCheckable(true);
    m_searchOrAction->setChecked(m_bOR);

    menu->addSeparator();

    menu->addAction(tr("Input names from selection"))->setData(INPUT_NAMES_FROM_SELECTION);

    // Add the search mode, search result handling, and match type actions to
    // exclusive action groups so that only one can be checked at a time, and
    // you can't un-check the active action
    m_searchModeActionGroup = new QActionGroup(this);
    m_searchModeActionGroup->setExclusive(true);
    m_searchModeActionGroup->addAction(m_searchByNameAction);
    m_searchModeActionGroup->addAction(m_searchByTypeAction);
    m_searchModeActionGroup->addAction(m_searchByAssetAction);
    m_searchResultHandlingActionGroup = new QActionGroup(this);
    m_searchResultHandlingActionGroup->setExclusive(true);
    m_searchResultHandlingActionGroup->addAction(m_searchHideOthersAction);
    m_searchResultHandlingActionGroup->addAction(m_searchFreezeOthersAction);
    m_searchResultHandlingActionGroup->addAction(m_searchJustSelectAction);
    m_searchMatchTypeActionGroup = new QActionGroup(this);
    m_searchMatchTypeActionGroup->setExclusive(true);
    m_searchMatchTypeActionGroup->addAction(m_searchAndAction);
    m_searchMatchTypeActionGroup->addAction(m_searchOrAction);

    return menu;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetTitle(const QString& title)
{
    m_title = title;
    m_ui->m_titleBtn->setText(m_title);

    const bool searchVisible =
        title == QLatin1String("Perspective") && !GetIEditor()->IsNewViewportInteractionModelEnabled();

    m_ui->m_viewportSearch->setVisible(searchVisible);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMaximize()
{
    if (m_pViewPane)
    {
        m_pViewPane->ToggleMaximize();
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnToggleHelpers()
{
    Helpers::ToggleHelpers();
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnToggleDisplayInfo()
{
    int currentDisplayInfo = gEnv->pConsole->GetCVar("r_displayInfo")->GetIVal();
    gEnv->pConsole->GetCVar("r_displayInfo")->Set(currentDisplayInfo >= 3 ? 0 : currentDisplayInfo + 1);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::AddFOVMenus(QMenu* menu, std::function<void(float)> callback, const QStringList& customPresets)
{
    static const float fovs[] = {
        10.0f,
        20.0f,
        40.0f,
        55.0f,
        60.0f,
        70.0f,
        80.0f,
        90.0f
    };

    static const size_t fovCount = sizeof(fovs) / sizeof(fovs[0]);

    for (size_t i = 0; i < fovCount; ++i)
    {
        const float fov = fovs[i];
        QAction* action = menu->addAction(QString::number(fov));
        connect(action, &QAction::triggered, action, [fov, callback](){ callback(fov); });
    }

    menu->addSeparator();

    if (!customPresets.empty())
    {
        for (size_t i = 0; i < customPresets.size(); ++i)
        {
            if (customPresets[i].isEmpty())
            {
                break;
            }

            float fov = gSettings.viewports.fDefaultFov;
            bool ok;
            float f = customPresets[i].toDouble(&ok);
            if (ok)
            {
                fov = std::max(1.0f, f);
                fov = std::min(120.0f, f);
                QAction* action = menu->addAction(customPresets[i]);
                connect(action, &QAction::triggered, action, [fov, callback](){ callback(fov); });
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuFOVCustom()
{
    bool ok;
    int fov = QInputDialog::getInt(this, tr("Custom FOV"), QString(), 60, 1, 120, 1, &ok);

    if (ok)
    {
        m_pViewPane->SetViewportFOV(fov);

        // Update the custom presets.
        const QString text = QString::number(fov);
        UpdateCustomPresets(text, m_customFOVPresets);
        SaveCustomPresets("FOVPresets", "FOVPreset", m_customFOVPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::CreateFOVMenu()
{
    if (!m_fovMenu)
    {
        m_fovMenu = new QMenu(this);
    }

    m_fovMenu->clear();

    AddFOVMenus(m_fovMenu, [this](float f) { m_pViewPane->SetViewportFOV(f); }, m_customFOVPresets);
    if (!m_fovMenu->isEmpty())
    {
        m_fovMenu->addSeparator();
    }

    QAction* action = m_fovMenu->addAction(tr("Custom..."));
    connect(action, &QAction::triggered, this, &CViewportTitleDlg::OnMenuFOVCustom);
}

void CViewportTitleDlg::PopUpFOVMenu()
{
    if (m_pViewPane == NULL)
    {
        return;
    }

    CreateFOVMenu();
    m_fovMenu->exec(QCursor::pos());
}

QMenu* const CViewportTitleDlg::GetFovMenu()
{
    CreateFOVMenu();
    return m_fovMenu;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::AddAspectRatioMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets)
{
    static const std::pair<unsigned int, unsigned int> ratios[] = {
        std::pair<unsigned int, unsigned int>(16, 9),
        std::pair<unsigned int, unsigned int>(16, 10),
        std::pair<unsigned int, unsigned int>(4, 3),
        std::pair<unsigned int, unsigned int>(5, 4)
    };

    static const size_t ratioCount = sizeof(ratios) / sizeof(ratios[0]);

    for (size_t i = 0; i < ratioCount; ++i)
    {
        int width = ratios[i].first;
        int height = ratios[i].second;
        QAction* action = menu->addAction(QString("%1:%2").arg(width).arg(height));
        connect(action, &QAction::triggered, action, [width, height, callback]() {callback(width, height); });
    }

    menu->addSeparator();

    for (size_t i = 0; i < customPresets.size(); ++i)
    {
        if (customPresets[i].isEmpty())
        {
            break;
        }

        static QRegularExpression regex(QStringLiteral("^(\\d+):(\\d+)$"));
        QRegularExpressionMatch matches = regex.match(customPresets[i]);
        if (matches.hasMatch())
        {
            bool ok;
            unsigned int width = matches.captured(1).toInt(&ok);
            Q_ASSERT(ok);
            unsigned int height = matches.captured(2).toInt(&ok);
            Q_ASSERT(ok);
            QAction* action = menu->addAction(customPresets[i]);
            connect(action, &QAction::triggered, action, [width, height, callback]() {callback(width, height); });
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuAspectRatioCustom()
{
    const QRect viewportRect = m_pViewPane->GetViewport()->rect();
    const unsigned int width = viewportRect.width();
    const unsigned int height = viewportRect.height();

    int whGCD = gcd(width, height);
    CCustomAspectRatioDlg aspectRatioInputDialog(width / whGCD, height / whGCD, this);

    if (aspectRatioInputDialog.exec() == QDialog::Accepted)
    {
        const unsigned int aspectX = aspectRatioInputDialog.GetX();
        const unsigned int aspectY = aspectRatioInputDialog.GetY();

        m_pViewPane->SetAspectRatio(aspectX, aspectY);

        // Update the custom presets.
        UpdateCustomPresets(QString::fromLatin1("%1:%2").arg(aspectX).arg(aspectY), m_customAspectRatioPresets);
        SaveCustomPresets("AspectRatioPresets", "AspectRatioPreset", m_customAspectRatioPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::CreateAspectMenu()
{
    if (!m_aspectMenu)
    {
        m_aspectMenu = new QMenu(this);
    }

    m_aspectMenu->clear();

    AddAspectRatioMenus(m_aspectMenu, [this](int width, int height) {m_pViewPane->SetAspectRatio(width, height); }, m_customAspectRatioPresets);
    if (!m_aspectMenu->isEmpty())
    {
        m_aspectMenu->addSeparator();
    }

    QAction* customAction = m_aspectMenu->addAction(tr("Custom..."));
    connect(customAction, &QAction::triggered, this, &CViewportTitleDlg::OnMenuAspectRatioCustom);
}

void CViewportTitleDlg::PopUpAspectMenu()
{
    if (!m_pViewPane)
    {
        return;
    }

    CreateAspectMenu();
    m_aspectMenu->exec(QCursor::pos());
}

QMenu* const CViewportTitleDlg::GetAspectMenu()
{
    CreateAspectMenu();
    return m_aspectMenu;
}

void CViewportTitleDlg::AddResolutionMenus(QMenu* menu, std::function<void(int, int)> callback, const QStringList& customPresets)
{
    static const CRenderViewport::SResolution resolutions[] = {
        CRenderViewport::SResolution(1280, 720),
        CRenderViewport::SResolution(1920, 1080),
        CRenderViewport::SResolution(2560, 1440),
        CRenderViewport::SResolution(2048, 858),
        CRenderViewport::SResolution(1998, 1080),
        CRenderViewport::SResolution(3840, 2160)
    };

    static const size_t resolutionCount = sizeof(resolutions) / sizeof(resolutions[0]);

    for (size_t i = 0; i < resolutionCount; ++i)
    {
        const int width = resolutions[i].width;
        const int height = resolutions[i].height;
        const QString text = QString::fromLatin1("%1 x %2").arg(width).arg(height);
        QAction* action = menu->addAction(text);
        connect(action, &QAction::triggered, action, [width, height, callback](){ callback(width, height); });
    }

    menu->addSeparator();

    for (size_t i = 0; i < customPresets.size(); ++i)
    {
        if (customPresets[i].isEmpty())
        {
            break;
        }

        static QRegularExpression regex(QStringLiteral("^(\\d+) x (\\d+)$"));
        QRegularExpressionMatch matches = regex.match(customPresets[i]);
        if (matches.hasMatch())
        {
            bool ok;
            int width = matches.captured(1).toInt(&ok);
            Q_ASSERT(ok);
            int height = matches.captured(2).toInt(&ok);
            Q_ASSERT(ok);
            QAction* action = menu->addAction(customPresets[i]);
            connect(action, &QAction::triggered, action, [width, height, callback](){ callback(width, height); });
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnMenuResolutionCustom()
{
    const QRect rectViewport = m_pViewPane->GetViewport()->rect();
    CCustomResolutionDlg resDlg(rectViewport.width(), rectViewport.height(), parentWidget());
    if (resDlg.exec() == QDialog::Accepted)
    {
        m_pViewPane->ResizeViewport(resDlg.GetWidth(), resDlg.GetHeight());
        // Update the custom presets.
        const QString text = QString::fromLatin1("%1 x %2").arg(resDlg.GetWidth()).arg(resDlg.GetHeight());
        UpdateCustomPresets(text, m_customResPresets);
        SaveCustomPresets("ResPresets", "ResPreset", m_customResPresets);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::CreateResolutionMenu()
{
    if (!m_resolutionMenu)
    {
        m_resolutionMenu = new QMenu(this);
    }

    m_resolutionMenu->clear();

    AddResolutionMenus(m_resolutionMenu, [this](int width, int height) { m_pViewPane->ResizeViewport(width, height); }, m_customResPresets);
    if (!m_resolutionMenu->isEmpty())
    {
        m_resolutionMenu->addSeparator();
    }

    QAction* action = m_resolutionMenu->addAction(tr("Custom..."));
    connect(action, &QAction::triggered, this, &CViewportTitleDlg::OnMenuResolutionCustom);
}

void CViewportTitleDlg::PopUpResolutionMenu()
{
    if (!m_pViewPane)
    {
        return;
    }

    CreateResolutionMenu();
    m_resolutionMenu->exec(QCursor::pos());
}

QMenu* const CViewportTitleDlg::GetResolutionMenu()
{
    CreateResolutionMenu();
    return m_resolutionMenu;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnViewportSizeChanged(int width, int height)
{
    m_ui->m_sizeStaticCtrl->setText(QString::fromLatin1("%1 x %2").arg(width).arg(height));

    if (width != 0 && height != 0)
    {
        // Calculate greatest common divider of width & height
        int whGCD = gcd(width, height);

        m_ui->m_ratioStaticCtrl->setText(QString::fromLatin1("%1:%2").arg(width / whGCD).arg(height / whGCD));
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnViewportFOVChanged(float fov)
{
    const float degFOV = RAD2DEG(fov);
    if (m_ui &&  m_ui->m_fovStaticCtrl)
    {
        m_ui->m_fovStaticCtrl->setText(QString::fromLatin1("%1%2").arg(qRound(degFOV)).arg(QString(QByteArray::fromPercentEncoding("%C2%B0"))));
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnDisplayRenderUpdate:
        m_ui->m_toggleHelpersBtn->setChecked(GetIEditor()->GetDisplaySettings()->IsDisplayHelpers());
        break;
    }
}

void CViewportTitleDlg::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    if (event == ESYSTEM_EVENT_RESIZE)
    {
        if (m_pViewPane)
        {
            const int eventWidth = static_cast<int>(wparam);
            const int eventHeight = static_cast<int>(lparam);
            const QWidget* viewport = m_pViewPane->GetViewport();

            // This should eventually be converted to an EBus to make it easy to connect to the correct viewport 
            // sending the event.  But for now, just detect that we've gotten width/height values that match our 
            // associated viewport
            if (viewport && (eventWidth == viewport->width()) && (eventHeight == viewport->height()))
            {
                OnViewportSizeChanged(eventWidth, eventHeight);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::InputNamesToSearchFromSelection()
{
    IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
    CSelectionGroup* pSelected = pObjMgr->GetSelection();
    QString names;
    for (int i = 0; i < pSelected->GetCount(); ++i)
    {
        names += pSelected->GetObject(i)->GetName();
        names += QStringLiteral(" ");
    }
    m_ui->m_viewportSearch->setText(names);
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnViewportSearchButtonClicked(const QAction* clickedAction)
{
    int cmd = clickedAction ? clickedAction->data().toInt() : 0;

    #define ASSIGN(dst, src) do { if ((dst) != (src)) { (dst) = (src); OnSearchTermChange(); } \
    }                                                                                          \
    while (0)

    if (cmd == SEARCH_BY_NAME)
    {
        ASSIGN(m_searchMode, ESM_BY_NAME);
    }
    else if (cmd == SEARCH_BY_TYPE)
    {
        ASSIGN(m_searchMode, ESM_BY_TYPE);
    }
    else if (cmd == SEARCH_BY_ASSET)
    {
        ASSIGN(m_searchMode, ESM_BY_ASSET);
    }
    else if (cmd == HIDE_OTHERS)
    {
        ASSIGN(m_searchResultHandling, ESRH_HIDE_OTHERS);
    }
    else if (cmd == FREEZE_OTHERS)
    {
        ASSIGN(m_searchResultHandling, ESRH_FREEZE_OTHERS);
    }
    else if (cmd == JUST_SELLECT)
    {
        ASSIGN(m_searchResultHandling, ESRH_JUST_SELECT);
    }
    else if (cmd == LOGICAL_AND)
    {
        ASSIGN(m_bOR, false);
    }
    else if (cmd == LOGICAL_OR)
    {
        ASSIGN(m_bOR, true);
    }
    else if (cmd == INPUT_NAMES_FROM_SELECTION)
    {
        InputNamesToSearchFromSelection();
        m_bOR = true;
        m_searchOrAction->setChecked(true);
        OnSearchTermChange();
    }

    UpdateSearchOptionsText();
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnViewportSearchClear()
{
    UnhideUnfreezeAll();
    //clear edit control
    gSettings.bIsSearchFilterActive = false;
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SetFocusToSearchField()
{
    if (m_ui->m_viewportSearch->isVisible())
    {
        m_ui->m_viewportSearch->setFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::OnSearchTermChange()
{
    // Get individual search terms.
    const QString searchTerm = m_ui->m_viewportSearch->text();

    if (!searchTerm.isEmpty())
    {
        gSettings.bIsSearchFilterActive = true;
    }
    else
    {
        gSettings.bIsSearchFilterActive = false;
    }

    const int MIN_CHARS_AS_A_SEARCH_TERM = 3;
    QStringList terms;

    QStringList splitTerms = searchTerm.split(QStringLiteral(" "));
    for (const QString& term : splitTerms)
    {
        if (term.length() >= MIN_CHARS_AS_A_SEARCH_TERM)
        {
            terms.append(term);
        }
    }

    if (terms.isEmpty())
    {
        UnhideUnfreezeAll();
        return;
    }

    // Make sure to lower case all terms because later we lower case all inputs to compare against
    for (QString& term : terms)
    {
        term = term.toLower();
    }

    // Perform a search.
    if (m_searchMode == ESM_BY_NAME)
    {
        SearchByName(terms);
    }
    else if (m_searchMode == ESM_BY_TYPE)
    {
        SearchByType(terms);
    }
    else if (m_searchMode == ESM_BY_ASSET)
    {
        SearchByAsset(terms);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::UnhideUnfreezeAll()
{
    IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
    int objCount = pObjMgr->GetObjectCount();
    CBaseObjectsArray objects;
    pObjMgr->GetObjects(objects);
    for (int i = 0; i < objCount; ++i)
    {
        CBaseObject* pObject = objects[i];
        pObjMgr->FreezeObject(pObject, false);
        pObjMgr->HideObject(pObject, false);
    }
}

//////////////////////////////////////////////////////////////////////////
static bool DoesTextSatisfyTerms(const QString& text, const QStringList& terms, bool bOR)
{
    if (bOR) // OR
    {
        for (const QString& term : terms)
        {
            if (text.contains(term))
            {
                return true;
            }
        }
        return false;
    }
    else // AND
    {
        for (const QString& term : terms)
        {
            if (!text.contains(term))
            {
                return false;
            }
        }
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////
static void HandleMatched(CBaseObject* pObject)
{
    IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();

    pObjMgr->FreezeObject(pObject, false);
    pObjMgr->HideObject(pObject, false);
    pObjMgr->SelectObject(pObject);
}

//////////////////////////////////////////////////////////////////////////
static void HandleFiltered(CBaseObject* pObject, CViewportTitleDlg::ESearchResultHandling searchResultHandling)
{
    IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
    pObjMgr->UnselectObject(pObject);
    if (searchResultHandling == CViewportTitleDlg::ESRH_HIDE_OTHERS)
    {
        pObjMgr->FreezeObject(pObject, false);
        pObjMgr->HideObject(pObject, true);
    }
    else if (searchResultHandling == CViewportTitleDlg::ESRH_FREEZE_OTHERS)
    {
        pObjMgr->HideObject(pObject, false);
        pObjMgr->FreezeObject(pObject, true);
    }
    else if (searchResultHandling == CViewportTitleDlg::ESRH_JUST_SELECT)
    {
        pObjMgr->FreezeObject(pObject, false);
        pObjMgr->HideObject(pObject, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SearchByType(const QStringList& terms)
{
    IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
    int objCount = pObjMgr->GetObjectCount();
    CBaseObjectsArray objects;
    pObjMgr->GetObjects(objects);
    for (int i = 0; i < objCount; ++i)
    {
        CBaseObject* pObject = objects[i];
        QString type = pObject->GetTypeDescription();
        if (DoesTextSatisfyTerms(type.toLower(), terms, m_bOR))
        {
            HandleMatched(pObject);
        }
        else
        {
            HandleFiltered(pObject, m_searchResultHandling);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SearchByName(const QStringList& terms)
{
    IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
    int objCount = pObjMgr->GetObjectCount();
    CBaseObjectsArray objects;
    pObjMgr->GetObjects(objects);
    for (int i = 0; i < objCount; ++i)
    {
        CBaseObject* pObject = objects[i];
        QString name = pObject->GetName();
        if (DoesTextSatisfyTerms(name.toLower(), terms, m_bOR))
        {
            HandleMatched(pObject);
        }
        else
        {
            HandleFiltered(pObject, m_searchResultHandling);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::SearchByAsset(const QStringList& terms)
{
    IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
    int objCount = pObjMgr->GetObjectCount();
    CBaseObjectsArray objects;
    pObjMgr->GetObjects(objects);
    for (int i = 0; i < objCount; ++i)
    {
        CBaseObject* pObject = objects[i];
        CUsedResources usedAssets;
        pObject->GatherUsedResources(usedAssets);
        CUsedResources::TResourceFiles::const_iterator itr = usedAssets.files.begin(),
                                                       end = usedAssets.files.end();
        bool bMatch = false;
        for (; itr != end; ++itr)
        {
            QString filename = *itr;
            if (DoesTextSatisfyTerms(filename.toLower(), terms, m_bOR))
            {
                bMatch = true;
                break;
            }
        }
        if (bMatch)
        {
            HandleMatched(pObject);
        }
        else
        {
            HandleFiltered(pObject, m_searchResultHandling);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CViewportTitleDlg::UpdateSearchOptionsText()
{
    QString str = "";

    if (m_searchMode == ESM_BY_NAME)
    {
        str += tr("By Name");
    }
    else if (m_searchMode == ESM_BY_TYPE)
    {
        str += tr("By Type");
    }
    else //if(m_searchMode == ESM_BY_ASSET)
    {
        str += tr("By Asset");
    }

    if (m_searchResultHandling == ESRH_HIDE_OTHERS)
    {
        str += tr(", Hide filtered");
    }
    else if (m_searchResultHandling == ESRH_FREEZE_OTHERS)
    {
        str += tr(", Freeze filtered");
    }
    else if (m_searchResultHandling == ESRH_JUST_SELECT)
    {
        str += tr(", Just Select");
    }

    if (m_bOR)
    {
        str += tr(", OR");
    }
    else
    {
        str += tr(", AND");
    }

    m_ui->m_viewportSearch->setPlaceholderText(str);
}

void CViewportTitleDlg::LoadCustomPresets(const QString& section, const QString& keyName, QStringList& outCustompresets)
{
    QSettings settings("Amazon", "O3DE"); // Temporary solution until we have the global Settings class.
    settings.beginGroup(section);
    outCustompresets = settings.value(keyName).toStringList();
    settings.endGroup();
}

void CViewportTitleDlg::SaveCustomPresets(const QString& section, const QString& keyName, const QStringList& custompresets)
{
    QSettings settings("Amazon", "O3DE"); // Temporary solution until we have the global Settings class.
    settings.beginGroup(section);
    settings.setValue(keyName, custompresets);
    settings.endGroup();
}

void CViewportTitleDlg::UpdateCustomPresets(const QString& text, QStringList& custompresets)
{
    custompresets.removeAll(text);
    custompresets.push_front(text);
    if (custompresets.size() > MAX_NUM_CUSTOM_PRESETS)
    {
        custompresets.erase(custompresets.begin() + MAX_NUM_CUSTOM_PRESETS, custompresets.end()); // QList doesn't have resize()
    }
}

void CViewportTitleDlg::OnChangedDisplayInfo([[maybe_unused]] ICVar* pDisplayInfo, QAbstractButton* pDisplayInfoButton)
{
    if (pDisplayInfoButton)
    {
        pDisplayInfoButton->setChecked(gEnv->pConsole->GetCVar("r_displayInfo")->GetIVal());
    }
}

bool CViewportTitleDlg::eventFilter(QObject* object, QEvent* event)
{
    bool consumeEvent = false;

    // These events are forwarded from the toolbar that took ownership of our widgets
    if (event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            OnMaximize();
            consumeEvent = true;
        }
    }
    else if (event->type() == QEvent::FocusIn)
    {
        parentWidget()->setFocus();
    }

    return QWidget::eventFilter(object, event) || consumeEvent;
}


namespace
{
    void PyToggleHelpers()
    {
        GetIEditor()->GetDisplaySettings()->DisplayHelpers(!GetIEditor()->GetDisplaySettings()->IsDisplayHelpers());
        GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);

        if (GetIEditor()->GetDisplaySettings()->IsDisplayHelpers() == false)
        {
            GetIEditor()->GetObjectManager()->SendEvent(EVENT_HIDE_HELPER);
        }
    }

    bool PyIsHelpersShown()
    {
        return GetIEditor()->GetDisplaySettings()->IsDisplayHelpers();
    }
}

namespace AzToolsFramework
{
    void ViewportTitleDlgPythonFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("toggle_helpers", PyToggleHelpers, nullptr, "Toggles the display of helpers."));
            addLegacyGeneral(behaviorContext->Method("is_helpers_shown", PyIsHelpersShown, nullptr, "Gets the display state of helpers."));
        }
    }
}

#include <moc_ViewportTitleDlg.cpp>

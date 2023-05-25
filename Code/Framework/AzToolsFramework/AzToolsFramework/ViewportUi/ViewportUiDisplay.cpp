/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportUi/ViewportUiCluster.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplay.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplayLayout.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>
#include <AzToolsFramework/ViewportUi/ViewportUiTextField.h>
#include <QWidget>

namespace AzToolsFramework::ViewportUi::Internal
{
    const static int HighlightBorderSize = ViewportUiLeftRightBottomBorderSize;
    const static char* const HighlightBorderColor = "#4A90E2";
    const static int HighlightBorderBackButtonIconSize = 20;
    const static char* const HighlightBorderBackButtonIconFile = "X_axis.svg";

    ViewportUiElementInfo::ViewportUiElementInfo() = default;
    ViewportUiElementInfo::~ViewportUiElementInfo() = default;

    ViewportUiElementInfo::ViewportUiElementInfo(AZStd::shared_ptr<QWidget> widget,
        ViewportUiElementId elementId, bool anchored)
        : m_widget(AZStd::move(widget))
        , m_viewportUiElementId(elementId)
        , m_anchored(anchored)
    {
    }

    static void UnparentWidgets(ViewportUiElementIdInfoLookup& viewportUiElementIdInfoLookup)
    {
        for (auto& element : viewportUiElementIdInfoLookup)
        {
            if (element.second.m_widget)
            {
                element.second.m_widget->setParent(nullptr);
            }
        }
    }

    static Qt::Alignment GetQtAlignment(const Alignment alignment)
    {
        switch (alignment)
        {
        case Alignment::TopRight:
            return Qt::AlignTop | Qt::AlignRight;
        case Alignment::TopLeft:
            return Qt::AlignTop | Qt::AlignLeft;
        case Alignment::BottomRight:
            return Qt::AlignBottom | Qt::AlignRight;
        case Alignment::BottomLeft:
            return Qt::AlignBottom | Qt::AlignLeft;
        case Alignment::Top:
            return Qt::AlignTop;
        case Alignment::Bottom:
            return Qt::AlignBottom;
        }

        AZ_Assert(false, "ViewportUI", "Unhandled ViewportUI Alignment %d", static_cast<int>(alignment));
        return Qt::AlignTop;
    }

    ViewportUiDisplay::ViewportUiDisplay(QWidget* parent, QWidget* renderOverlay)
        : m_renderOverlay(renderOverlay)
        , m_uiMainWindow(parent)
        , m_uiOverlay(parent)
        , m_fullScreenLayout(&m_uiOverlay)
        , m_uiOverlayLayout()
        , m_viewportBorderText(&m_uiOverlay)
        , m_viewportBorderBackButton(&m_uiOverlay)
    {
        AzFramework::ViewportImGuiNotificationBus::Handler::BusConnect();
    }

    ViewportUiDisplay::~ViewportUiDisplay()
    {
        AzFramework::ViewportImGuiNotificationBus::Handler::BusDisconnect();
        UnparentWidgets(m_viewportUiElements);
    }

    void ViewportUiDisplay::AddCluster(AZStd::shared_ptr<ButtonGroup> buttonGroup, const Alignment alignment)
    {
        if (!buttonGroup.get())
        {
            return;
        }

        auto viewportUiCluster = AZStd::make_shared<ViewportUiCluster>(buttonGroup);
        auto id = AddViewportUiElement(viewportUiCluster);
        buttonGroup->SetViewportUiElementId(id);
        PositionViewportUiElementAnchored(id, GetQtAlignment(alignment));
    }

    void ViewportUiDisplay::AddClusterButton(const ViewportUiElementId clusterId, Button* button)
    {
        if (auto viewportUiCluster = qobject_cast<ViewportUiCluster*>(GetViewportUiElement(clusterId).get()))
        {
            viewportUiCluster->RegisterButton(button);
        }
    }

    void ViewportUiDisplay::SetClusterButtonLocked(const ViewportUiElementId clusterId, const ButtonId buttonId, const bool isLocked)
    {
        if (auto viewportUiCluster = qobject_cast<ViewportUiCluster*>(GetViewportUiElement(clusterId).get()))
        {
            viewportUiCluster->SetButtonLocked(buttonId, isLocked);
        }
    }

    void ViewportUiDisplay::SetClusterButtonTooltip(
        const ViewportUiElementId clusterId, const ButtonId buttonId, const AZStd::string& tooltip)
    {
        if (auto viewportUiCluster = qobject_cast<ViewportUiCluster*>(GetViewportUiElement(clusterId).get()))
        {
            viewportUiCluster->SetButtonTooltip(buttonId, tooltip);
        }
    }

    void ViewportUiDisplay::RemoveClusterButton(ViewportUiElementId clusterId, ButtonId buttonId)
    {
        if (auto cluster = qobject_cast<ViewportUiCluster*>(GetViewportUiElement(clusterId).get()))
        {
            cluster->RemoveButton(buttonId);
        }
    }

    void ViewportUiDisplay::UpdateCluster(ViewportUiElementId clusterId)
    {
        if (auto cluster = qobject_cast<ViewportUiCluster*>(GetViewportUiElement(clusterId).get()))
        {
            cluster->Update();
        }
    }

    void ViewportUiDisplay::AddSwitcher(AZStd::shared_ptr<ButtonGroup> buttonGroup, const Alignment alignment)
    {
        if (!buttonGroup.get())
        {
            return;
        }

        auto viewportUiSwitcher = AZStd::make_shared<ViewportUiSwitcher>(buttonGroup);
        auto id = AddViewportUiElement(viewportUiSwitcher);
        buttonGroup->SetViewportUiElementId(id);
        PositionViewportUiElementAnchored(id, GetQtAlignment(alignment));
    }

    void ViewportUiDisplay::AddSwitcherButton(const ViewportUiElementId switcherId, Button* button)
    {
        if (auto switcher = qobject_cast<ViewportUiSwitcher*>(GetViewportUiElement(switcherId).get()))
        {
            switcher->AddButton(button);
        }
    }

    void ViewportUiDisplay::RemoveSwitcherButton(ViewportUiElementId switcherId, ButtonId buttonId)
    {
        if (auto switcher = qobject_cast<ViewportUiSwitcher*>(GetViewportUiElement(switcherId).get()))
        {
            switcher->RemoveButton(buttonId);
        }
    }

    void ViewportUiDisplay::UpdateSwitcher(ViewportUiElementId switcherId)
    {
        if (auto switcher = qobject_cast<ViewportUiSwitcher*>(GetViewportUiElement(switcherId).get()))
        {
            switcher->Update();
        }
    }

    void ViewportUiDisplay::SetSwitcherActiveButton(ViewportUiElementId switcherId, ButtonId buttonId)
    {
        if (auto viewportUiSwitcher = qobject_cast<ViewportUiSwitcher*>(GetViewportUiElement(switcherId).get()))
        {
            viewportUiSwitcher->SetActiveButton(buttonId);
        }
    }

    void ViewportUiDisplay::SetSwitcherButtonTooltip(
        const ViewportUiElementId switcherId, const ButtonId buttonId, const AZStd::string& tooltip)
    {
        if (auto viewportUiSwitcher = qobject_cast<ViewportUiSwitcher*>(GetViewportUiElement(switcherId).get()))
        {
            viewportUiSwitcher->SetButtonTooltip(buttonId, tooltip);
        }
    }

    void ViewportUiDisplay::AddTextField(AZStd::shared_ptr<TextField> textField)
    {
        if (!textField.get())
        {
            return;
        }

        auto viewportUiTextField = AZStd::make_shared<ViewportUiTextField>(textField);
        auto id = AddViewportUiElement(viewportUiTextField);
        textField->m_viewportId = id;
        PositionViewportUiElementAnchored(id, Qt::AlignTop | Qt::AlignLeft);
    }

    void ViewportUiDisplay::UpdateTextField(const ViewportUiElementId textFieldId)
    {
        if (auto textField = qobject_cast<ViewportUiTextField*>(GetViewportUiElement(textFieldId).get()))
        {
            textField->Update();
        }
    }

    void ViewportUiDisplay::Update()
    {
        for (const auto& element : m_viewportUiElements)
        {
            const ViewportUiElementInfo& elementInfo = element.second;
            if (!elementInfo.m_anchored)
            {
                const auto screenPoint =
                    AzFramework::WorldToScreen(elementInfo.m_worldPosition, AzToolsFramework::GetCameraState(m_viewportId));
                elementInfo.m_widget->move(screenPoint.m_x, screenPoint.m_y);
            }
        }

        PositionUiOverlayOverRenderViewport();
    }

    ViewportUiElementId ViewportUiDisplay::AddViewportUiElement(AZStd::shared_ptr<QWidget> widget)
    {
        if (!widget)
        {
            return InvalidViewportUiElementId;
        }

        ViewportUiElementId newId = ViewportUiElementId(++m_numViewportElements);
        ViewportUiElementInfo newElement{ widget, newId, true };
        m_viewportUiElements.insert({ newId, newElement });
        return newId;
    }

    AZStd::shared_ptr<QWidget> ViewportUiDisplay::GetViewportUiElement(ViewportUiElementId elementId)
    {
        if (auto element = m_viewportUiElements.find(elementId); element != m_viewportUiElements.end())
        {
            return element->second.m_widget;
        }

        return nullptr;
    }

    ViewportUiElementId ViewportUiDisplay::GetViewportUiElementId(QPointer<QWidget> widget)
    {
        if (auto element = AZStd::find_if(
                m_viewportUiElements.begin(), m_viewportUiElements.end(),
                [widget](const auto& it)
                {
                    return it.second.m_widget.get() == widget;
                });
            element != m_viewportUiElements.end())
        {
            return element->second.m_viewportUiElementId;
        }

        return InvalidViewportUiElementId;
    }

    void ViewportUiDisplay::RemoveViewportUiElement(ViewportUiElementId elementId)
    {
        AZ_Assert(
            elementId != AzToolsFramework::ViewportUi::InvalidViewportUiElementId,
            "Tried to remove a Viewport UI element using an invalid or removed ViewportUiElementId.");

        auto viewportUiMapElement = m_viewportUiElements.find(elementId);
        if (viewportUiMapElement != m_viewportUiElements.end())
        {
            viewportUiMapElement->second.m_widget->hide();
            viewportUiMapElement->second.m_widget->setParent(nullptr);
            m_viewportUiElements.erase(viewportUiMapElement);
        }
    }

    bool ViewportUiDisplay::UiDisplayEnabled() const
    {
        return m_renderOverlay && m_renderOverlay->isVisible();
    }

    void ViewportUiDisplay::ShowViewportUiElement(ViewportUiElementId elementId)
    {
        if (ViewportUiElementInfo element = GetViewportUiElementInfo(elementId); element.m_widget)
        {
            element.m_widget->show();
        }
    }

    void ViewportUiDisplay::HideViewportUiElement(ViewportUiElementId elementId)
    {
        if (ViewportUiElementInfo element = GetViewportUiElementInfo(elementId); element.m_widget)
        {
            element.m_widget->hide();
        }
    }

    bool ViewportUiDisplay::IsViewportUiElementVisible(ViewportUiElementId elementId)
    {
        if (ViewportUiElementInfo element = GetViewportUiElementInfo(elementId); element.m_widget)
        {
            return element.IsValid() && element.m_widget->isVisible();
        }

        return false;
    }

    QMargins ViewportUiDisplay::ViewportElementMargins() const
    {
        if (m_imGuiActive)
        {
            return GetViewportBorderVisible() ? ViewportUiOverlayImGuiBorderMargin : ViewportUiOverlayImGuiMargin;
        }
        else
        {
            return GetViewportBorderVisible() ? ViewportUiOverlayBorderMargin : ViewportUiOverlayDefaultMargin;
        }
    }

    void ViewportUiDisplay::CreateViewportBorder(
        const AZStd::string& borderTitle, AZStd::optional<ViewportUiBackButtonCallback> backButtonCallback)
    {
        m_uiOverlay.setStyleSheet(QString("border: %1px solid %2; border-top: %3px solid %4;")
                                      .arg(
                                          QString::number(HighlightBorderSize), HighlightBorderColor,
                                          QString::number(ViewportUiTopBorderSize), HighlightBorderColor));

        m_viewportBorderText.setAlignment(Qt::AlignCenter);

        m_viewportBorderText.show();
        m_uiOverlayLayout.setContentsMargins(ViewportElementMargins());
        ChangeViewportBorderText(borderTitle.c_str());

        // only display the back button if a callback was provided
        m_viewportBorderBackButtonCallback = backButtonCallback;
        m_viewportBorderBackButton.setVisible(m_viewportBorderBackButtonCallback.has_value());
    }

    void ViewportUiDisplay::ChangeViewportBorderText(const char* borderTitle)
    {
        // when the text changes if the width is different it will flicker as it changes,
        // this sets the width to the entire overlay to avoid that
        m_viewportBorderText.setFixedWidth(m_uiOverlay.width());
        m_viewportBorderText.setText(borderTitle);
    }

    void ViewportUiDisplay::OnImGuiActivated()
    {
        m_imGuiActive = true;
        m_uiOverlayLayout.setContentsMargins(ViewportElementMargins());
    };

    void ViewportUiDisplay::OnImGuiDeactivated()
    {
        m_imGuiActive = false;
        m_uiOverlayLayout.setContentsMargins(ViewportElementMargins());
    };

    void ViewportUiDisplay::RemoveViewportBorder()
    {
        m_viewportBorderText.hide();
        m_uiOverlay.setStyleSheet("border: none;");
        m_uiOverlayLayout.setContentsMargins(ViewportElementMargins());
        m_viewportBorderBackButtonCallback.reset();
        m_viewportBorderBackButton.hide();
    }

    bool ViewportUiDisplay::GetViewportBorderVisible() const
    {
        return m_viewportBorderText.isVisible();
    }

    void ViewportUiDisplay::PositionViewportUiElementFromWorldSpace(ViewportUiElementId elementId, const AZ::Vector3& pos)
    {
        auto viewportUiMapElement = m_viewportUiElements.find(elementId);

        if (viewportUiMapElement != m_viewportUiElements.end() && viewportUiMapElement->second.m_widget)
        {
            viewportUiMapElement->second.m_anchored = false;
            viewportUiMapElement->second.m_worldPosition = pos;

            SetUiOverlayContents(viewportUiMapElement->second.m_widget.get());
        }
    }

    void ViewportUiDisplay::PositionViewportUiElementAnchored(ViewportUiElementId elementId, const Qt::Alignment alignment)
    {
        auto viewportUiMapElement = m_viewportUiElements.find(elementId);

        if (viewportUiMapElement != m_viewportUiElements.end() && viewportUiMapElement->second.m_widget)
        {
            viewportUiMapElement->second.m_anchored = true;
            SetUiOverlayContentsAnchored(viewportUiMapElement->second.m_widget.get(), alignment);
        }
    }

    // disables system background for widget and gives a transparent background
    static void ConfigureWindowForViewportUi(QPointer<QMainWindow> mainWindow)
    {
        // no background for the widget else each set of buttons/text-fields/etc would have a black box around them
        SetTransparentBackground(mainWindow);
        mainWindow->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    }

    void ViewportUiDisplay::InitializeUiOverlay()
    {
        m_uiMainWindow.setObjectName(QString("ViewportUiWindow"));
        ConfigureWindowForViewportUi(&m_uiMainWindow);
        m_uiMainWindow.hide();

        m_uiOverlay.setObjectName(QString("ViewportUiOverlay"));
        m_uiMainWindow.setCentralWidget(&m_uiOverlay);
        m_uiOverlay.hide();

        // remove any spacing and margins from the UI Overlay Layout
        m_fullScreenLayout.setSpacing(0);
        m_fullScreenLayout.setContentsMargins(0, 0, 0, 0);
        m_fullScreenLayout.addLayout(&m_uiOverlayLayout, 0, 0, 1, 1);

        // style the label which will appear on top of the highlight border
        m_viewportBorderText.setStyleSheet(QString("background-color: %1; border: none").arg(HighlightBorderColor));
        m_viewportBorderText.setFixedHeight(ViewportUiTopBorderSize);
        m_viewportBorderText.hide();
        m_fullScreenLayout.addWidget(&m_viewportBorderText, 0, 0, Qt::AlignTop | Qt::AlignHCenter);

        m_viewportBorderBackButton.setAutoRaise(true); // hover highlight
        m_viewportBorderBackButton.hide();

        QIcon backButtonIcon(QString(":/stylesheet/img/UI20/toolbar/%1").arg(HighlightBorderBackButtonIconFile));
        m_viewportBorderBackButton.setIcon(backButtonIcon);
        m_viewportBorderBackButton.setIconSize(QSize(HighlightBorderBackButtonIconSize, HighlightBorderBackButtonIconSize));

        // setup the handler for the back button to call the user provided callback (if any)
        QObject::connect(
            &m_viewportBorderBackButton, &QToolButton::clicked,
            [this]
            {
                if (m_viewportBorderBackButtonCallback.has_value())
                {
                    // we need to swap out the existing back button callback because it will be reset in RemoveViewportBorder()
                    // so preserve the lifetime with this temporary callback until after the call to RemoveViewportBorder()
                    AZStd::optional<ViewportUiBackButtonCallback> backButtonCallback;
                    m_viewportBorderBackButtonCallback.swap(backButtonCallback);
                    RemoveViewportBorder();
                    (*backButtonCallback)();
                }
            });
        m_fullScreenLayout.addWidget(&m_viewportBorderBackButton, 0, 0, Qt::AlignTop | Qt::AlignRight);
    }

    void ViewportUiDisplay::PrepareWidgetForViewportUi(QPointer<QWidget> widget)
    {
        widget->setAttribute(Qt::WA_ShowWithoutActivating);
        widget->setParent(&m_uiOverlay);
        widget->setStyleSheet("border: none;");
    }

    void ViewportUiDisplay::SetUiOverlayContents(QPointer<QWidget> widget)
    {
        if (!widget)
        {
            return;
        }

        PrepareWidgetForViewportUi(widget);
    }

    void ViewportUiDisplay::SetUiOverlayContentsAnchored(QPointer<QWidget> widget, const Qt::Alignment alignment)
    {
        if (!widget)
        {
            return;
        }

        PrepareWidgetForViewportUi(widget);
        m_uiOverlayLayout.AddAnchoredWidget(widget, alignment);
    }

    void ViewportUiDisplay::UpdateUiOverlayGeometry()
    {
        // add the viewport border region if visible
        QRegion region;
        if (m_viewportBorderText.isVisible())
        {
            // get the border region by taking the entire region and subtracting the non-border area
            region += m_uiOverlay.rect();
            region -= QRect(
                QPoint(m_uiOverlay.rect().left() + HighlightBorderSize, m_uiOverlay.rect().top() + ViewportUiTopBorderSize),
                QPoint(m_uiOverlay.rect().right() - HighlightBorderSize, m_uiOverlay.rect().bottom() - HighlightBorderSize));

            // if the user changes the size of their window, release the width of the border so the
            // overlay can resize
            if (m_viewportBorderText.width() != m_renderOverlay->width())
            {
                m_viewportBorderText.setMinimumWidth(0);
                m_viewportBorderText.setMaximumWidth(m_renderOverlay->width());
            }
        }

        // add all children widget regions
        region += m_uiOverlay.childrenRegion();

        // set viewport ui visibility depending on if elements are present
        const bool visible = !region.isEmpty() && UiDisplayEnabled();
        m_uiMainWindow.setVisible(visible);
        m_uiOverlay.setVisible(visible);

        m_uiMainWindow.setMask(region);
    }

    void ViewportUiDisplay::PositionUiOverlayOverRenderViewport()
    {
        QPoint offset = m_renderOverlay->mapToGlobal(QPoint());
        m_uiMainWindow.setGeometry(offset.x(), offset.y(), m_renderOverlay->width(), m_renderOverlay->height());
        m_uiOverlay.setGeometry(m_uiMainWindow.rect());
        UpdateUiOverlayGeometry();
    }

    ViewportUiElementInfo ViewportUiDisplay::GetViewportUiElementInfo(const ViewportUiElementId elementId)
    {
        if (auto element = m_viewportUiElements.find(elementId); element != m_viewportUiElements.end())
        {
            return element->second;
        }

        return ViewportUiElementInfo{ nullptr, InvalidViewportUiElementId, false };
    }

    const QMainWindow* ViewportUiDisplay::GetUiMainWindow() const
    {
        return &m_uiMainWindow;
    }

    const QWidget* ViewportUiDisplay::GetUiOverlay() const
    {
        return &m_uiOverlay;
    }

    const QGridLayout* ViewportUiDisplay::GetUiOverlayLayout() const
    {
        return &m_uiOverlayLayout;
    }

    void SetTransparentBackground(QWidget* widget)
    {
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setAutoFillBackground(false);
    }
} // namespace AzToolsFramework::ViewportUi::Internal

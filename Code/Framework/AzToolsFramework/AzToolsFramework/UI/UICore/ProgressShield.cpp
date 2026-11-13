/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/chrono.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <AzToolsFramework/UI/UICore/ui_ProgressShield.h>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    // ----------------------------------------------------------------------------------
    // The sheets of the shield, and the elements that carry them in the flattened GUI
    //
    // The form gave the shield one sheet (ProgressShield.ui:17-22 before the flattening: a gradient behind the window and the colour of
    // its labels) and its frame another (ProgressShield.ui:56-61: a translucent card with a white line). The legacy GUI sets them after
    // setupUi (StyleManager::setLegacyStyleSheet), written from the constants below; the flattened GUI sets none, and the elements
    // below give the shield, its frame and its label the same look. The shield is a plain widget class of its own, which the style sheet
    // style gives no styled background: its gradient reached the screen as the Window brush of its palette, which paints the translucent
    // window (BoxElement::m_fillsPalette).
    // ----------------------------------------------------------------------------------

    //! Everything private to this file that the flattened GUI adds. A named namespace, not an anonymous one: a unity build joins files
    //! into one translation unit, where anonymous namespaces merge.
    namespace ProgressShieldFlatInternal
    {
        // ---- The colours and the sizes of the sheets
        //! ProgressShield.ui:17 `stop:0`. TOKENIZE: ProgressShieldTop
        constexpr QColor ShieldTop(0, 0, 0, 50);
        //! ProgressShield.ui:17 `stop:1`. TOKENIZE: ProgressShieldBottom
        constexpr QColor ShieldBottom(0, 0, 0, 90);
        //! ProgressShield.ui:21 `QLabel { color }`. TOKENIZE: ProgressShieldText
        constexpr QColor LabelColor(0, 0, 0, 255);
        //! ProgressShield.ui:58 `background-color` of the frame. TOKENIZE: ProgressShieldFrameFill
        constexpr QColor FrameFill(255, 255, 255, 128);
        //! ProgressShield.ui:59 `border: 2px solid white`. TOKENIZE: ProgressShieldFrameLine
        constexpr QColor FrameLine(255, 255, 255);
        constexpr int FrameLineWidth = 2;               // ProgressShield.ui:59. TOKENIZE: ProgressShieldFrameLineWidth
        constexpr int FrameRadius = 10;                 // ProgressShield.ui:60. TOKENIZE: ProgressShieldFrameRadius

        constexpr const char* ShieldName = "progressShield";                        // ProgressShield.ui:17 `#progressShield`
        constexpr const char* FrameName = "frame";                                  // ProgressShield.ui:56 `QFrame#frame`
        constexpr const char* ShieldClass = "AzToolsFramework::ProgressShield";

        // ---- The sheets, as the form had them
        QString rgbaOf(const QColor& color)
        {
            return QStringLiteral("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
        }

        //! ProgressShield.ui:17-22: a gradient behind the shield, and the colour of the labels in it
        QString shieldSheet()
        {
            return QStringLiteral(
                       "QWidget #progressShield { background-color:qlineargradient(spread:reflect, x1:0, y1:0, x2:0, y2:1, "
                       "stop:0 %1, stop:1 %2);  }\n\nQLabel\n{\ncolor:%3;\n}\n")
                .arg(rgbaOf(ShieldTop), rgbaOf(ShieldBottom), rgbaOf(LabelColor));
        }

        //! ProgressShield.ui:56-61: the frame in the middle of the shield
        QString frameSheet()
        {
            return QStringLiteral("QFrame#frame \n{\nbackground-color: %1;\nborder: %2px solid %3;\nborder-radius: %4px;\n}")
                .arg(rgbaOf(FrameFill))
                .arg(FrameLineWidth)
                .arg(FrameLine.name())
                .arg(FrameRadius);
        }

        // ---- The elements
        //! `QWidget #progressShield { background-color: qlineargradient }`, ProgressShield.ui:17: the gradient, from the top to the bottom
        //! of the window. TOKENIZE: ProgressShieldBackground
        const AzQtComponents::BoxElement ShieldBox{
            .m_name = "Editor.Box.ProgressShield",
            .m_idle = { .m_fill = AzQtComponents::BoxElement::vertical(ShieldTop, ShieldBottom) },
            .m_fillsPalette = true,
        };

        //! `QFrame#frame`, ProgressShield.ui:56-61: a translucent fill and a line of its own around it, with round corners.
        //! TOKENIZE: ProgressShieldFrame
        const AzQtComponents::BoxElement FrameBox{
            .m_name = "Editor.Box.ProgressShieldFrame",
            .m_nativeBorder = false,                    // ProgressShield.ui:59 a border of its own
            .m_radius = FrameRadius,                    // ProgressShield.ui:60
            .m_idle = {
                .m_fill = QBrush(FrameFill),            // ProgressShield.ui:58
                .m_edges = AzQtComponents::BoxElement::border(FrameLineWidth, FrameLine),   // ProgressShield.ui:59
            },
            .m_fillsPalette = true,                     // ProgressShield.ui:58: the fill is the frame's Base, Button and Window
        };

        //! `QLabel { color }`, ProgressShield.ui:19-22, over the text of every label (Text.qss:91-92: the family and the size an element
        //! restates, as it takes the place of the Global one). TOKENIZE: ProgressShieldLabel
        const AzQtComponents::TextElement ProgressLabel{
            .m_name = "Editor.Text.ProgressShieldLabel",
            .m_family = AzQtComponents::Foundations::Label::Family,         // Text.qss:91
            .m_pixelSize = AzQtComponents::Foundations::Label::Size,        // Text.qss:92
            .m_color = LabelColor,                                          // ProgressShield.ui:21
        };

        // ---- The selectors
        //! `QWidget #progressShield`: the shield, which has a parent (the descendant combinator wants a widget above it)
        bool isShield(const QWidget* widget)
        {
            return AzQtComponents::Selectors::named(widget, ShieldName) && widget->parentWidget();
        }

        //! `QFrame#frame` of the sheet the frame set on itself: the frame of a shield
        bool isFrame(const QWidget* widget)
        {
            return AzQtComponents::Selectors::named(widget, FrameName) && AzQtComponents::Selectors::withinClass(widget, ShieldClass);
        }

        //! `QLabel` of the shield's sheet: every label below the shield. The form has one, in the frame (ProgressShield.ui:70), and the
        //! shield adds none: the parent and the widget above it are looked at, where a walk to the root ran for every label of the Editor
        bool isLabel(const QWidget* widget)
        {
            const QWidget* parent = widget->parentWidget();
            return parent && (qobject_cast<const ProgressShield*>(parent) || qobject_cast<const ProgressShield*>(parent->parentWidget()));
        }

        //! The first shield built in the flattened GUI registers the elements, once
        void registerFlatElements()
        {
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                AzQtComponents::BoxElement::registerElement(&ShieldBox, &ProgressShield::staticMetaObject, &isShield);
                AzQtComponents::BoxElement::registerElement(&FrameBox, &QFrame::staticMetaObject, &isFrame);
                AzQtComponents::TextElement::registerElement(&ProgressLabel, &QLabel::staticMetaObject, &isLabel);
            }
        }
    } // namespace ProgressShieldFlatInternal

    namespace ProgressShieldInternal
    {
        void PumpEvents()
        {
            // re: 16 msec - pump QueuedEvents at a min of ~60 FPS
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents, 16);
            AZ::TickBus::ExecuteQueuedEvents();
        }
    } // namespace ProgressShieldInternal

    ProgressShield::ProgressShield(QWidget* pParent)
        : QWidget(pParent, Qt::WindowFlags(Qt::ToolTip | Qt::BypassWindowManagerHint | Qt::FramelessWindowHint))
    {
        AZ_Assert(pParent && parentWidget(), "There must be a parent.");
        if (pParent && parentWidget())
        {
            // The flattened GUI's elements come first: they cover the widgets from their first polish
            if (AzQtComponents::StyleManager::stylesheetsDisabled())
            {
                ProgressShieldFlatInternal::registerFlatElements();
            }

            uiConstructor = azcreate(Ui::progressShield, ());
            uiConstructor->setupUi(this);

            // The sheets the form had, for the legacy GUI only; the flattened GUI's elements give the widgets the same look
            AzQtComponents::StyleManager::setLegacyStyleSheet(this, ProgressShieldFlatInternal::shieldSheet());
            AzQtComponents::StyleManager::setLegacyStyleSheet(uiConstructor->frame, ProgressShieldFlatInternal::frameSheet());

            setAttribute(Qt::WA_TranslucentBackground);
            setAttribute(Qt::WA_TransparentForMouseEvents);
            setAttribute(Qt::WA_NoSystemBackground);
            setGeometry(parentWidget()->geometry()); // because we're a tooltip, we work in absolute coordinates.
            pParent->installEventFilter(this);
            setProgress(0, 0, tr("Loading..."));
            UpdateGeometry();
        }
    }

    ProgressShield::~ProgressShield()
    {
        parentWidget()->removeEventFilter(this);
        azdestroy(uiConstructor);
    }

    void ProgressShield::LegacyShowAndWait(QWidget* parent, QString label, AZStd::function<bool(int& current, int& max)> completeCallback, int delayMS)
    {
        using AzSysClock = AZStd::chrono::steady_clock;
        using AzMillisec = AZStd::chrono::milliseconds;
        bool showShield = false;

        // delay showing the progress shield when we have callbacks that complete quickly
        int current = 0, max = 0;
        AzSysClock::time_point start = AzSysClock::now();
        while (!completeCallback(current, max))
        {
            ProgressShieldInternal::PumpEvents();
            if (AZStd::chrono::duration_cast<AzMillisec>(AzSysClock::now() - start) > AzMillisec(delayMS))
            {
                showShield = true;
                break;
            }
        }

        // we've exceeded the delay, show the progress shield and wait for callback to complete
        if (showShield)
        {
            ProgressShield shield(parent);
            current = max = 0;

            shield.show();
            while (!completeCallback(current, max))
            {
                shield.setProgress(current, max, label);
                ProgressShieldInternal::PumpEvents();
            }
        }
    }

    void ProgressShield::UpdateGeometry()
    {
        setGeometry(parentWidget()->geometry()); // because we're a tooltip window (not a child), we work in absolute coordinates.
    }


    bool ProgressShield::eventFilter(QObject* obj, QEvent* event)
    {
        if ((event->type() == QEvent::Resize) || (event->type() == QEvent::Move))
        {
            UpdateGeometry();
        }

        // hand it to the owner
        return QObject::eventFilter(obj, event);
    }

    void ProgressShield::setProgress(int current, int max, const QString& label)
    {
        uiConstructor->progressLabel->setText(label);
        uiConstructor->progressLabel->show();
        uiConstructor->progressBar->setMaximum(max);
        uiConstructor->progressBar->setValue(current);
    }

} // namespace AzToolsFramework


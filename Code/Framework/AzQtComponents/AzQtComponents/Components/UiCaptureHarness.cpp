/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/UiCaptureHarness.h>

#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QDynamicPropertyChangeEvent>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFontInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMetaEnum>
#include <QMetaProperty>
#include <QPixmap>
#include <QPushButton>
#include <QSet>
#include <QStackedWidget>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QWidget>

namespace AzQtComponents
{
    namespace
    {
        // Same settle rule as the Gallery harness: minimum dwell, then two identical grabs.
        constexpr int SettleMinDwellMs = 3000;
        constexpr int SettleProbeMs = 500;
        constexpr int SettleMaxMs = 10000;
        constexpr int DefaultStartupDelayMs = 20000;
        constexpr const char* RequestPropertyName = "o3deUiCapture";
        constexpr const char* WalkPropertyName = "o3deUiWalk";

        // ------------------------------------------------------------------------------
        // Environment
        // ------------------------------------------------------------------------------

        QString environment(const char* name)
        {
            return QString::fromLocal8Bit(qgetenv(name));
        }

        const QSet<QString>& recordedPropertyNames()
        {
            static const QSet<QString> names = []()
            {
                QSet<QString> result;
                QFile file(environment("O3DE_UI_CAPTURE_ATTRIBUTES"));
                if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    QTextStream in(&file);
                    while (!in.atEnd())
                    {
                        const QString line = in.readLine().trimmed();
                        if (!line.isEmpty())
                        {
                            result.insert(line);
                        }
                    }
                }
                return result;
            }();
            return names;
        }

        // ------------------------------------------------------------------------------
        // Widget description
        // ------------------------------------------------------------------------------

        void pumpEventsFor(int milliseconds)
        {
            QEventLoop loop;
            QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
            loop.exec();
        }

        bool drawnThroughStyleSheet(const QWidget* widget)
        {
            return widget->style() && widget->style()->inherits("QStyleSheetStyle");
        }

        QString propertyText(const QObject* widget, const char* name)
        {
            const QVariant value = widget->property(name);
            const int index = widget->metaObject()->indexOfProperty(name);
            if (index >= 0)
            {
                const QMetaProperty metaProperty = widget->metaObject()->property(index);
                if (metaProperty.isEnumType())
                {
                    const QMetaEnum metaEnum = metaProperty.enumerator();
                    const QByteArray key = metaEnum.isFlag() ? metaEnum.valueToKeys(value.toInt()) : QByteArray(metaEnum.valueToKey(value.toInt()));
                    return QStringLiteral("%1|%2").arg(value.toInt()).arg(QString::fromLatin1(key));
                }
            }
            if (value.typeId() == QMetaType::QStringList || value.typeId() == QMetaType::QVariantList)
            {
                return value.toStringList().join(QLatin1Char(' '));
            }
            return value.toString();
        }

        QJsonObject describeProperties(const QObject* widget)
        {
            QJsonObject properties;
            for (const QByteArray& name : widget->dynamicPropertyNames())
            {
                if (name != RequestPropertyName && !name.startsWith("_q_"))
                {
                    properties.insert(QString::fromLatin1(name), propertyText(widget, name.constData()));
                }
            }
            for (const QString& name : recordedPropertyNames())
            {
                const QByteArray latin = name.toLatin1();
                if (widget->metaObject()->indexOfProperty(latin.constData()) >= 0)
                {
                    properties.insert(name, propertyText(widget, latin.constData()));
                }
            }
            return properties;
        }

        QJsonObject describeFlags(const QWidget* widget)
        {
            QJsonObject flags;
            flags.insert(QStringLiteral("enabled"), widget->isEnabled());
            flags.insert(QStringLiteral("focus"), widget->hasFocus());
            flags.insert(QStringLiteral("window"), widget->isWindow());
            flags.insert(QStringLiteral("activeWindow"), widget->isActiveWindow());
            flags.insert(QStringLiteral("underMouse"), widget->underMouse());
            flags.insert(QStringLiteral("autoFill"), widget->autoFillBackground());
            if (const auto* button = qobject_cast<const QAbstractButton*>(widget))
            {
                flags.insert(QStringLiteral("checkable"), button->isCheckable());
                flags.insert(QStringLiteral("checked"), button->isChecked());
                flags.insert(QStringLiteral("down"), button->isDown());
            }
            if (const auto* pushButton = qobject_cast<const QPushButton*>(widget))
            {
                flags.insert(QStringLiteral("flat"), pushButton->isFlat());
                flags.insert(QStringLiteral("default"), pushButton->isDefault());
            }
            if (const auto* lineEdit = qobject_cast<const QLineEdit*>(widget))
            {
                flags.insert(QStringLiteral("readOnly"), lineEdit->isReadOnly());
            }
            if (const auto* comboBox = qobject_cast<const QComboBox*>(widget))
            {
                flags.insert(QStringLiteral("editable"), comboBox->isEditable());
            }
            return flags;
        }

        QJsonObject describePalette(const QWidget* widget)
        {
            static const QList<QPair<QPalette::ColorRole, const char*>> roles = {
                { QPalette::Window, "Window" },     { QPalette::WindowText, "WindowText" },
                { QPalette::Base, "Base" },         { QPalette::AlternateBase, "AlternateBase" },
                { QPalette::Text, "Text" },         { QPalette::Button, "Button" },
                { QPalette::ButtonText, "ButtonText" }, { QPalette::Highlight, "Highlight" },
                { QPalette::HighlightedText, "HighlightedText" }, { QPalette::PlaceholderText, "PlaceholderText" },
                { QPalette::Link, "Link" },
            };
            QJsonObject palette;
            const QPalette& source = widget->palette();
            for (const auto& role : roles)
            {
                palette.insert(QString::fromLatin1(role.second), source.color(QPalette::Active, role.first).name(QColor::HexArgb));
                palette.insert(QStringLiteral("disabled.") + QString::fromLatin1(role.second),
                    source.color(QPalette::Disabled, role.first).name(QColor::HexArgb));
            }
            return palette;
        }

        QJsonObject describeFont(const QWidget* widget)
        {
            const QFont& font = widget->font();
            QJsonObject description;
            description.insert(QStringLiteral("family"), font.family());
            description.insert(QStringLiteral("resolvedFamily"), QFontInfo(font).family());
            description.insert(QStringLiteral("px"), font.pixelSize());
            description.insert(QStringLiteral("pt"), font.pointSizeF());
            description.insert(QStringLiteral("weight"), font.weight());
            description.insert(QStringLiteral("italic"), font.italic());
            return description;
        }

        // ------------------------------------------------------------------------------
        // Metrics: what explains a widget's size, position, fill and font, and the parts
        // it draws that are not widgets (tabs, menu items, header sections, view rows)
        // ------------------------------------------------------------------------------

        QJsonArray sizeArray(const QSize& size)
        {
            // QWIDGETSIZE_MAX reads as -1: no limit
            const auto side = [](int value) { return value >= QWIDGETSIZE_MAX ? -1 : value; };
            return QJsonArray{ side(size.width()), side(size.height()) };
        }

        QJsonArray marginsArray(const QMargins& margins)
        {
            return QJsonArray{ margins.left(), margins.top(), margins.right(), margins.bottom() };
        }

        QJsonArray rectArray(const QRect& rect)
        {
            return QJsonArray{ rect.x(), rect.y(), rect.width(), rect.height() };
        }

        QString shortText(const QString& text)
        {
            const QString simple = text.simplified();
            return simple.size() > 80 ? simple.left(77) + QStringLiteral("...") : simple;
        }

        QString widgetText(const QWidget* widget)
        {
            if (const auto* label = qobject_cast<const QLabel*>(widget))
            {
                return label->text();
            }
            if (const auto* button = qobject_cast<const QAbstractButton*>(widget))
            {
                return button->text();
            }
            if (const auto* groupBox = qobject_cast<const QGroupBox*>(widget))
            {
                return groupBox->title();
            }
            if (const auto* lineEdit = qobject_cast<const QLineEdit*>(widget))
            {
                return lineEdit->text().isEmpty() ? lineEdit->placeholderText() : lineEdit->text();
            }
            return widget->isWindow() ? widget->windowTitle() : QString();
        }

        QJsonArray describeParts(const QWidget* widget)
        {
            QJsonArray parts;
            if (const auto* tabBar = qobject_cast<const QTabBar*>(widget))
            {
                for (int index = 0; index < tabBar->count() && index < 40; ++index)
                {
                    parts.append(QJsonObject{ { QStringLiteral("kind"), QStringLiteral("tab") },
                        { QStringLiteral("rect"), rectArray(tabBar->tabRect(index)) },
                        { QStringLiteral("text"), shortText(tabBar->tabText(index)) },
                        { QStringLiteral("current"), index == tabBar->currentIndex() } });
                }
            }
            else if (const auto* menu = qobject_cast<const QMenu*>(widget))
            {
                const QList<QAction*> actions = menu->actions();
                for (int index = 0; index < actions.size() && index < 80; ++index)
                {
                    QAction* action = actions.at(index);
                    if (action->isVisible())
                    {
                        parts.append(QJsonObject{ { QStringLiteral("kind"), action->isSeparator() ? QStringLiteral("separator") : QStringLiteral("item") },
                            { QStringLiteral("rect"), rectArray(menu->actionGeometry(action)) },
                            { QStringLiteral("text"), shortText(action->text()) } });
                    }
                }
            }
            else if (const auto* header = qobject_cast<const QHeaderView*>(widget))
            {
                for (int index = 0; index < header->count() && index < 40; ++index)
                {
                    if (!header->isSectionHidden(index))
                    {
                        parts.append(QJsonObject{ { QStringLiteral("kind"), QStringLiteral("section") },
                            { QStringLiteral("position"), header->sectionViewportPosition(index) },
                            { QStringLiteral("size"), header->sectionSize(index) } });
                    }
                }
            }
            else if (const auto* view = qobject_cast<const QAbstractItemView*>(widget))
            {
                if (const QAbstractItemModel* model = view->model())
                {
                    const QModelIndex root = view->rootIndex();
                    for (int row = 0; row < model->rowCount(root) && row < 8; ++row)
                    {
                        const QRect rect = view->visualRect(model->index(row, 0, root));
                        if (rect.isValid())
                        {
                            parts.append(QJsonObject{ { QStringLiteral("kind"), QStringLiteral("row") }, { QStringLiteral("rect"), rectArray(rect) } });
                        }
                    }
                }
            }
            return parts;
        }

        void describeMetrics(const QWidget* widget, QJsonObject& row)
        {
            const QSizePolicy policy = widget->sizePolicy();
            row.insert(QStringLiteral("sizes"), QJsonObject{ { QStringLiteral("hint"), sizeArray(widget->sizeHint()) },
                { QStringLiteral("minHint"), sizeArray(widget->minimumSizeHint()) },
                { QStringLiteral("min"), sizeArray(widget->minimumSize()) },
                { QStringLiteral("max"), sizeArray(widget->maximumSize()) },
                { QStringLiteral("policy"), QJsonArray{ int(policy.horizontalPolicy()), int(policy.verticalPolicy()) } } });
            row.insert(QStringLiteral("contents"), marginsArray(widget->contentsMargins()));

            if (const QLayout* layout = widget->layout())
            {
                const auto* grid = qobject_cast<const QGridLayout*>(layout);
                row.insert(QStringLiteral("layout"), QJsonObject{ { QStringLiteral("class"), QString::fromLatin1(layout->metaObject()->className()) },
                    { QStringLiteral("margins"), marginsArray(layout->contentsMargins()) },
                    { QStringLiteral("spacing"), grid ? QJsonValue(QJsonArray{ grid->horizontalSpacing(), grid->verticalSpacing() }) : QJsonValue(layout->spacing()) } });
            }

            // Set by code on the widget itself (a font or palette not inherited, a styled background)
            static const QList<QPair<Qt::WidgetAttribute, const char*>> attributeNames = {
                { Qt::WA_StyledBackground, "StyledBackground" }, { Qt::WA_TranslucentBackground, "TranslucentBackground" },
                { Qt::WA_NoSystemBackground, "NoSystemBackground" }, { Qt::WA_OpaquePaintEvent, "OpaquePaintEvent" },
                { Qt::WA_SetFont, "SetFont" }, { Qt::WA_SetPalette, "SetPalette" }, { Qt::WA_Hover, "Hover" },
            };
            QJsonArray attributes;
            for (const auto& attribute : attributeNames)
            {
                if (widget->testAttribute(attribute.first))
                {
                    attributes.append(QString::fromLatin1(attribute.second));
                }
            }
            row.insert(QStringLiteral("attributes"), attributes);

            if (const auto* frame = qobject_cast<const QFrame*>(widget))
            {
                row.insert(QStringLiteral("frame"), QJsonObject{
                    { QStringLiteral("shape"), QString::fromLatin1(QMetaEnum::fromType<QFrame::Shape>().valueToKey(frame->frameShape())) },
                    { QStringLiteral("shadow"), QString::fromLatin1(QMetaEnum::fromType<QFrame::Shadow>().valueToKey(frame->frameShadow())) },
                    { QStringLiteral("line"), frame->lineWidth() }, { QStringLiteral("midLine"), frame->midLineWidth() },
                    { QStringLiteral("width"), frame->frameWidth() } });
            }

            const QString text = shortText(widgetText(widget));
            if (!text.isEmpty())
            {
                row.insert(QStringLiteral("text"), text);
            }
            if (const auto* button = qobject_cast<const QAbstractButton*>(widget))
            {
                row.insert(QStringLiteral("iconSize"), sizeArray(button->iconSize()));
            }
            else if (const auto* view = qobject_cast<const QAbstractItemView*>(widget))
            {
                row.insert(QStringLiteral("iconSize"), sizeArray(view->iconSize()));
            }

            const QJsonArray parts = describeParts(widget);
            if (!parts.isEmpty())
            {
                row.insert(QStringLiteral("parts"), parts);
            }
        }

        QString widgetPath(const QWidget* widget, int depth)
        {
            QStringList parts;
            for (; widget && depth > 0; widget = widget->parentWidget(), --depth)
            {
                const QString name = widget->objectName();
                parts.append(QString::fromLatin1(widget->metaObject()->className()) + (name.isEmpty() ? QString() : QStringLiteral("#") + name));
            }
            return parts.join(QStringLiteral(" < "));
        }

        // ------------------------------------------------------------------------------
        // Output
        // ------------------------------------------------------------------------------

        QJsonArray classChain(const QObject* object)
        {
            QJsonArray classes;
            for (const QMetaObject* metaObject = object->metaObject(); metaObject; metaObject = metaObject->superClass())
            {
                classes.append(QString::fromLatin1(metaObject->className()));
            }
            return classes;
        }

        void writeWidgetDump(QWidget* root, const QString& basePath, const QList<QWidget*>& widgets)
        {
            // Qt matches selectors and collects sheets up the QObject parent chain, which continues
            // above a grabbed window (a floating pane under the main window): those ancestors are
            // recorded too, with negative ids.
            QHash<const QObject*, int> ids;
            QJsonArray ancestors;
            int ancestorId = -2;
            for (const QObject* ancestor = root->parent(); ancestor; ancestor = ancestor->parent())
            {
                ids.insert(ancestor, ancestorId--);
            }
            for (const QObject* ancestor = root->parent(); ancestor; ancestor = ancestor->parent())
            {
                QJsonObject row;
                row.insert(QStringLiteral("id"), ids.value(ancestor));
                row.insert(QStringLiteral("parent"), ids.value(ancestor->parent(), -1));
                row.insert(QStringLiteral("outside"), true);
                row.insert(QStringLiteral("classes"), classChain(ancestor));
                row.insert(QStringLiteral("name"), ancestor->objectName());
                row.insert(QStringLiteral("properties"), describeProperties(ancestor));
                const QString sheet = ancestor->property("styleSheet").toString();
                if (!sheet.trimmed().isEmpty())
                {
                    row.insert(QStringLiteral("sheet"), sheet);
                }
                ancestors.append(row);
            }

            QJsonArray rows;
            for (QWidget* widget : widgets)
            {
                if (widget != root && !widget->isVisibleTo(root))
                {
                    continue;
                }
                const int id = static_cast<int>(rows.size());
                ids.insert(widget, id);
                const QPoint topLeft = widget == root ? QPoint(0, 0) : widget->mapTo(root, QPoint(0, 0));

                QJsonObject row;
                row.insert(QStringLiteral("id"), id);
                row.insert(QStringLiteral("parent"), ids.value(widget->parent(), -1));
                row.insert(QStringLiteral("classes"), classChain(widget));
                row.insert(QStringLiteral("name"), widget->objectName());
                row.insert(QStringLiteral("geometry"), QJsonArray{ topLeft.x(), topLeft.y(), widget->width(), widget->height() });
                row.insert(QStringLiteral("properties"), describeProperties(widget));
                row.insert(QStringLiteral("flags"), describeFlags(widget));
                row.insert(QStringLiteral("palette"), describePalette(widget));
                row.insert(QStringLiteral("font"), describeFont(widget));
                row.insert(QStringLiteral("sheetStyle"), drawnThroughStyleSheet(widget));
                if (!widget->styleSheet().trimmed().isEmpty())
                {
                    row.insert(QStringLiteral("sheet"), widget->styleSheet());
                }
                describeMetrics(widget, row);
                rows.append(row);
            }

            QJsonObject document;
            document.insert(QStringLiteral("app"), QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName());
            document.insert(QStringLiteral("screen"), QFileInfo(basePath).fileName());
            document.insert(QStringLiteral("mode"), StyleManager::stylesheetsDisabled() ? QStringLiteral("flat") : QStringLiteral("qss"));
            document.insert(QStringLiteral("appSheetChars"), qApp->styleSheet().size());
            document.insert(QStringLiteral("devicePixelRatio"), root->devicePixelRatioF());
            document.insert(QStringLiteral("widgets"), rows);
            document.insert(QStringLiteral("ancestors"), ancestors);

            QFile file(basePath + QStringLiteral(".widgets.json"));
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                file.write(QJsonDocument(document).toJson(QJsonDocument::Compact));
            }
        }

        void appendCensus(const QString& basePath, const QList<QWidget*>& widgets)
        {
            int sources = 0;
            int proxied = 0;
            QStringList sourceLines;
            for (const QWidget* widget : widgets)
            {
                const QString sheet = widget->styleSheet().simplified();
                if (!sheet.isEmpty())
                {
                    ++sources;
                    sourceLines.append(QStringLiteral("  %1 | %2").arg(widgetPath(widget, 4).left(90), -90).arg(sheet.left(160)));
                }
                proxied += drawnThroughStyleSheet(widget) ? 1 : 0;
            }

            QFile file(QFileInfo(basePath).absolutePath() + QStringLiteral("/__sheets.txt"));
            if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            {
                QTextStream out(&file);
                out << "== " << QFileInfo(basePath).fileName() << "  widgets=" << widgets.size() << "  sources=" << sources
                    << "  proxied=" << proxied << "  appSheetChars=" << qApp->styleSheet().size() << '\n';
                for (const QString& line : sourceLines)
                {
                    out << line << '\n';
                }
            }
        }

        QString fileSafe(QString text)
        {
            for (QChar& character : text)
            {
                if (!character.isLetterOrNumber())
                {
                    character = QLatin1Char('-');
                }
            }
            return text.left(48);
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Public entry points
    // ----------------------------------------------------------------------------------

    void UiCaptureHarness::installIfRequested(QApplication* application)
    {
        const QString outputDir = environment("O3DE_UI_CAPTURE_DIR");
        if (outputDir.isEmpty() || !application)
        {
            return;
        }
        QDir().mkpath(outputDir);

        // A blinking text cursor makes two captures of one screen differ by itself: keep it steady
        // (a flash time of 0 stops the blink, QApplication::setCursorFlashTime)
        QApplication::setCursorFlashTime(0);

        new UiCaptureHarness(application, outputDir);
    }

    void UiCaptureHarness::settle(QWidget* widget)
    {
        pumpEventsFor(SettleMinDwellMs);
        QImage previous = widget->grab().toImage();
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < SettleMaxMs)
        {
            pumpEventsFor(SettleProbeMs);
            const QImage current = widget->grab().toImage();
            if (current == previous)
            {
                return;
            }
            previous = current;
        }
        qWarning() << "UiCaptureHarness: did not settle within" << SettleMaxMs << "ms:" << widget->windowTitle();
    }

    void UiCaptureHarness::record(QWidget* root, const QString& basePath)
    {
        if (!root)
        {
            return;
        }
        QList<QWidget*> widgets = root->findChildren<QWidget*>();
        widgets.prepend(root);
        writeWidgetDump(root, basePath, widgets);
        appendCensus(basePath, widgets);

        // The exact application sheet Qt applies (imports expanded, variables substituted), once
        // per folder: the style resolution matches rules against the dumps with this text.
        const QString appSheet = qApp->styleSheet();
        QFile appSheetFile(QFileInfo(basePath).absolutePath() + QStringLiteral("/__appsheet.qss"));
        if (!appSheet.isEmpty() && !appSheetFile.exists() && appSheetFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            appSheetFile.write(appSheet.toUtf8());
        }
    }

    // ----------------------------------------------------------------------------------
    // Walker: every menu, tab, stacked page and hidden dock of a window. Nothing is triggered
    // (no action runs); every state is restored after its capture.
    // ----------------------------------------------------------------------------------

    namespace
    {
        constexpr int WalkMaxStates = 80;

        struct WalkState
        {
            QStringList m_screens;
            int m_count = 0;
        };

        void captureState(QWidget* widget, const QString& basePath, WalkState& state)
        {
            if (state.m_count >= WalkMaxStates)
            {
                return;
            }
            ++state.m_count;
            UiCaptureHarness::settle(widget);
            widget->grab().save(basePath + QStringLiteral(".png"));
            UiCaptureHarness::record(widget, basePath);
            state.m_screens.append(QFileInfo(basePath).fileName());
        }

        QString label(const QObject* object, const QString& text)
        {
            const QString name = !text.isEmpty() ? text
                : (!object->objectName().isEmpty() ? object->objectName() : QString::fromLatin1(object->metaObject()->className()));
            return fileSafe(name);
        }

        void walkMenu(QMenu* menu, const QPoint& globalPos, const QString& basePath, int depth, WalkState& state)
        {
            menu->popup(globalPos);
            pumpEventsFor(SettleProbeMs); // aboutToShow fills dynamic menus
            if (menu->isVisible())
            {
                captureState(menu, basePath, state);
                if (depth == 0)
                {
                    for (QAction* action : menu->actions())
                    {
                        QMenu* subMenu = action->menu();
                        if (!subMenu || !action->isVisible() || !action->isEnabled())
                        {
                            continue;
                        }
                        const QPoint subPos = menu->mapToGlobal(menu->actionGeometry(action).topRight());
                        walkMenu(subMenu, subPos, basePath + QLatin1Char('-') + label(subMenu, action->text()), depth + 1, state);
                    }
                }
            }
            menu->hide();
            pumpEventsFor(100);
        }

        void walkTabs(QWidget* window, QWidget* container, const QString& basePath, int depth, WalkState& state)
        {
            for (QTabWidget* tabs : container->findChildren<QTabWidget*>())
            {
                if (!tabs->isVisible())
                {
                    continue;
                }
                const int original = tabs->currentIndex();
                for (int index = 0; index < tabs->count(); ++index)
                {
                    if (index == original || !tabs->isTabEnabled(index) || !tabs->isTabVisible(index))
                    {
                        continue;
                    }
                    tabs->setCurrentIndex(index);
                    const QString name = basePath + QStringLiteral("__tab-") + label(tabs, QString()) + QLatin1Char('-')
                        + label(tabs->widget(index), tabs->tabText(index));
                    captureState(window, name, state);
                    if (depth == 0)
                    {
                        walkTabs(window, tabs->widget(index), name, depth + 1, state);
                    }
                }
                tabs->setCurrentIndex(original);
            }
        }
    } // namespace

    QStringList UiCaptureHarness::walk(QWidget* window, const QString& basePath)
    {
        WalkState state;

        // Menu bar menus, one level of submenus
        for (QMenuBar* menuBar : window->findChildren<QMenuBar*>())
        {
            if (!menuBar->isVisible())
            {
                continue;
            }
            for (QAction* action : menuBar->actions())
            {
                QMenu* menu = action->menu();
                if (!menu || !action->isVisible())
                {
                    continue;
                }
                const QPoint pos = menuBar->mapToGlobal(menuBar->actionGeometry(action).bottomLeft());
                walkMenu(menu, pos, basePath + QStringLiteral("__menu-") + label(menu, action->text()), 0, state);
            }
        }

        // Every other tab (one level of nested tab widgets)
        walkTabs(window, window, basePath, 0, state);

        // Every other page of a stacked widget (a QTabWidget's own stack is covered above)
        for (QStackedWidget* stack : window->findChildren<QStackedWidget*>())
        {
            if (!stack->isVisible() || qobject_cast<QTabWidget*>(stack->parentWidget()))
            {
                continue;
            }
            const int original = stack->currentIndex();
            for (int index = 0; index < stack->count(); ++index)
            {
                if (index == original)
                {
                    continue;
                }
                stack->setCurrentIndex(index);
                captureState(window, basePath + QStringLiteral("__page-") + label(stack, QString()) + QLatin1Char('-')
                    + label(stack->widget(index), QString()) + QString::number(index), state);
            }
            stack->setCurrentIndex(original);
        }

        // Docks of this window that are hidden
        for (QDockWidget* dock : window->findChildren<QDockWidget*>())
        {
            if (dock->isVisible() || dock->isFloating() || dock->window() != window)
            {
                continue;
            }
            dock->show();
            captureState(window, basePath + QStringLiteral("__dock-") + label(dock, dock->windowTitle()), state);
            dock->hide();
        }

        // The window's own list of captured states, in capture order
        QFile file(QFileInfo(basePath).absolutePath() + QStringLiteral("/__screens.txt"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            QTextStream out(&file);
            for (const QString& screen : state.m_screens)
            {
                out << screen << '\n';
            }
        }
        return state.m_screens;
    }

    // ----------------------------------------------------------------------------------
    // Triggers
    // ----------------------------------------------------------------------------------

    UiCaptureHarness::UiCaptureHarness(QObject* parent, const QString& outputDir)
        : QObject(parent)
        , m_outputDir(outputDir)
    {
        qApp->installEventFilter(this);

        if (environment("O3DE_UI_CAPTURE_AUTO") != QStringLiteral("0"))
        {
            bool parsed = false;
            int delay = environment("O3DE_UI_CAPTURE_DELAY_MS").toInt(&parsed);
            QTimer::singleShot(parsed ? delay : DefaultStartupDelayMs, this, &UiCaptureHarness::captureTopLevelWindows);
        }
    }

    bool UiCaptureHarness::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::DynamicPropertyChange)
        {
            const QByteArray name = static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName();
            auto* widget = qobject_cast<QWidget*>(watched);
            if (widget && (name == RequestPropertyName || name == WalkPropertyName))
            {
                const QString basePath = widget->property(name.constData()).toString();
                if (!basePath.isEmpty())
                {
                    if (name == RequestPropertyName)
                    {
                        record(widget, basePath);
                    }
                    else
                    {
                        walk(widget, basePath);
                    }
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }

    void UiCaptureHarness::captureTopLevelWindows()
    {
        const QString app = QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName();

        // The windows present at startup (the walk opens popups, which are top-level too)
        QList<QWidget*> windows;
        for (QWidget* window : QApplication::topLevelWidgets())
        {
            if (window->isVisible() && window->windowType() != Qt::ToolTip && window->windowType() != Qt::Popup)
            {
                windows.append(window);
            }
        }

        int index = 0;
        for (QWidget* window : windows)
        {
            const QString title = window->windowTitle().isEmpty() ? QString::fromLatin1(window->metaObject()->className()) : window->windowTitle();
            const QString name = QStringLiteral("%1_%2_%3").arg(app).arg(index++, 2, 10, QLatin1Char('0')).arg(fileSafe(title));
            const QString basePath = m_outputDir + QLatin1Char('/') + name;

            settle(window);
            window->grab().save(basePath + QStringLiteral(".png"));
            record(window, basePath);
            QFile file(m_outputDir + QStringLiteral("/__screens.txt"));
            if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            {
                QTextStream(&file) << name << '\n';
            }
            if (environment("O3DE_UI_CAPTURE_WALK") != QStringLiteral("0"))
            {
                walk(window, basePath);
            }
        }

        // Leave the way a user would: close the windows (the app's own shutdown path); quit only
        // if the app is still running afterwards (tray apps such as the Asset Processor).
        if (environment("O3DE_UI_CAPTURE_QUIT") != QStringLiteral("0"))
        {
            for (QWidget* window : windows)
            {
                window->close();
            }
            QTimer::singleShot(10000, qApp, &QCoreApplication::quit);
        }
    }
} // namespace AzQtComponents

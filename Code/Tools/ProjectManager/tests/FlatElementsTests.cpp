/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/FrameElement.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/TabElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Style.h>
#include <FlatElements.h>

#include <QApplication>
#include <QByteArray>
#include <QByteArrayList>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QMap>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScopedPointer>
#include <QScrollArea>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabBar>
#include <QTableWidget>
#include <QVariant>

#include <algorithm>
#include <utility>

// =====================================================================================================================
// The elements of FlatElements.cpp against the widgets of the legacy Project Manager.
//
// A widget is written as the selector of the sheet writes it: a chain "QDialogButtonBox#footer > QPushButton[primary=true]", each
// link a class, an object name (#) and the properties (the matchers compare a property as a string). A case builds the chain, polishes
// its last widget as the flattened style does (apply), and says which element it gets, or what the element carries:
// - one case for every element, from a representative Native widget: its chain is that of a widget captured in the legacy GUI (the
//   n2 widget dumps), or, where no capture reached, the place the code puts it; trimmed to the ancestors a matcher can read;
// - the cases a matcher could get wrong, where the widget is near the one it should select but is not: the same name in another
//   place, a property that is present and not true, a chain that is one link too long;
// - for each element the numbers it carries against the Native widget's: the box (margin, padding, border, limits, properties), the
//   face of every state, the contents offsets of a frame, the font and colour of a text element, the values of a tab bar.
// The Native values are the legacy GUI's: the n2 widget dumps (sizes, fonts, contents offsets, palettes) and, where a dump does not
// hold the value (the fills, edges and images of the boxes in each state), the cascade of the legacy sheets for the chain, which
// agrees with the dumps wherever both speak. The test executable does not run the flattened GUI (StyleManager::stylesheetsDisabled() is
// false in it): the elements are applied directly, as Style::polish applies them.
// =====================================================================================================================

namespace O3DE::ProjectManager
{
    // ----------------------------------------------------------------------------------
    // Widget chains
    // ----------------------------------------------------------------------------------

    namespace FlatElementsTestsInternal
    {
        template<typename W>
        QWidget* Create(QWidget* parent)
        {
            return new W(parent);
        }

        QWidget* CreateByClassName(const QString& className, QWidget* parent)
        {
            using Creator = QWidget* (*)(QWidget*);
            static const QMap<QString, Creator> creators{
                { QStringLiteral("QCheckBox"), &Create<QCheckBox> },
                { QStringLiteral("QComboBox"), &Create<QComboBox> },
                { QStringLiteral("QDialog"), &Create<QDialog> },
                { QStringLiteral("QDialogButtonBox"), &Create<QDialogButtonBox> },
                { QStringLiteral("QFrame"), &Create<QFrame> },
                { QStringLiteral("QLabel"), &Create<QLabel> },
                { QStringLiteral("QLineEdit"), &Create<QLineEdit> },
                { QStringLiteral("QListView"), &Create<QListView> },
                { QStringLiteral("QMainWindow"), &Create<QMainWindow> },
                { QStringLiteral("QProgressBar"), &Create<QProgressBar> },
                { QStringLiteral("QPushButton"), &Create<QPushButton> },
                { QStringLiteral("QRadioButton"), &Create<QRadioButton> },
                { QStringLiteral("QScrollArea"), &Create<QScrollArea> },
                { QStringLiteral("QStackedWidget"), &Create<QStackedWidget> },
                { QStringLiteral("QTabBar"), &Create<QTabBar> },
                { QStringLiteral("QTableWidget"), &Create<QTableWidget> },
                { QStringLiteral("QWidget"), &Create<QWidget> },
            };
            const auto creator = creators.constFind(className);
            return creator == creators.constEnd() ? nullptr : (*creator)(parent);
        }

        //! The widgets of a chain, owned by the first. The last is the one under test.
        class WidgetChain
        {
        public:
            explicit WidgetChain(const char* chain)
            {
                for (const QString& link : QString::fromLatin1(chain).split(QStringLiteral(" > ")))
                {
                    m_leaf = Link(link, m_leaf);
                    if (!m_root)
                    {
                        m_root.reset(m_leaf);
                    }
                }
            }

            QWidget* Leaf() const
            {
                return m_leaf;
            }

        private:
            //! "Class#objectName[property=value]..." as a child of parent
            static QWidget* Link(const QString& link, QWidget* parent)
            {
                const int propertiesAt = link.indexOf(QLatin1Char('['));
                const QString head = link.left(propertiesAt < 0 ? link.size() : propertiesAt);
                const int nameAt = head.indexOf(QLatin1Char('#'));
                QWidget* widget = CreateByClassName(head.left(nameAt < 0 ? head.size() : nameAt), parent);
                EXPECT_NE(widget, nullptr) << "no class for " << link.toUtf8().constData();
                if (!widget)
                {
                    return parent;
                }
                if (nameAt >= 0)
                {
                    widget->setObjectName(head.mid(nameAt + 1));
                }
                if (propertiesAt >= 0)
                {
                    for (const QString& property : link.mid(propertiesAt).split(QLatin1Char(']'), Qt::SkipEmptyParts))
                    {
                        const QStringList pair = property.mid(1).split(QLatin1Char('='));           // "[name=value"
                        widget->setProperty(pair.at(0).toUtf8().constData(), pair.at(1));
                    }
                }
                return widget;
            }

            QScopedPointer<QWidget> m_root;
            QWidget* m_leaf = nullptr;
        };

        // ------------------------------------------------------------------------------
        // Cases
        // ------------------------------------------------------------------------------

        //! The box element a widget gets: m_element is its Look (ProjectManager.Box.<Look>); "": none
        struct BoxCase
        {
            const char* m_element;
            const char* m_chain;
        };

        //! The font and colour a widget gets from its text element (Native's). A colour of nullptr: the element sets none.
        struct TextCase
        {
            const char* m_element;
            const char* m_chain;
            int m_pixelSize;
            int m_weight;
            bool m_italic;
            const char* m_color;
            const char* m_disabledColor;
        };

        // ------------------------------------------------------------------------------
        // One case for every box element, from the chain of a representative Native widget
        // ------------------------------------------------------------------------------

        const BoxCase BoxCases[] = {
            // ---- The window and the widgets of no particular place
            { "ProgressBar",
              "QWidget#GemCatalogCartOverlayGemDownloadHeader > QWidget#GemCatalogCartOverlayGemDownloadBG > "
              "QProgressBar#DownloadProgressBar" },
            { "ScreensCtrl", "QFrame > QMainWindow > QWidget#ScreensCtrl" },
            { "MainWindow", "QFrame > QMainWindow" },

            // ---- Screen chrome: headers, footers and surfaces
            { "FooterButtonPrimary", "QFrame > QDialogButtonBox#footer > QPushButton[primary=true]" },
            { "FooterButtonSecondary", "QFrame > QDialogButtonBox#footer > QPushButton[secondary=true]" },
            { "FooterButtonDanger", "QDialog#GemUninstallDialog > QDialogButtonBox#footer > QPushButton[danger=true]" },
            { "HeaderBackButton", "QFrame > QFrame#header > QPushButton" },
            { "HeaderTitle", "QFrame > QFrame#header > QLabel#headerTitle" },
            { "HeaderSubtitle", "QFrame > QFrame#header > QLabel#headerSubTitle" },
            { "Header", "QStackedWidget > QFrame > QFrame#header" },
            { "Footer", "QStackedWidget > QFrame > QDialogButtonBox#footer" },
            { "Body", "QStackedWidget > QFrame > QStackedWidget#body" },
            { "TopBar", "QFrame > QStackedWidget#body > QFrame#projectSettingsTopFrame" },

            // ---- The start page
            { "StartCardCreate", "QStackedWidget > QFrame#firstTimeContent > QPushButton#createProjectButton" },
            { "StartCardOpen", "QStackedWidget > QFrame#firstTimeContent > QPushButton#addProjectButton" },
            { "StartCardRemote", "QStackedWidget > QFrame#firstTimeContent > QPushButton#addRemoteProjectButton" },
            { "StartTitle", "QStackedWidget > QFrame#firstTimeContent > QLabel#titleLabel" },
            { "StartIntro", "QStackedWidget > QFrame#firstTimeContent > QLabel#introLabel" },

            // ---- The projects page and its cards
            { "ProjectCardCancel",
              "QFrame#projectButton > QLabel#labelButton > QWidget > QPushButton#projectActionCancelButton[danger=true]" },
            { "NewProjectButton", "QFrame#projectsContent > QFrame > QPushButton#newProjectButton" },
            { "ProjectsTitle", "QFrame#projectsContent > QFrame > QLabel#titleLabel" },
            { "ProjectsGrid", "QStackedWidget > QFrame#projectsContent > QScrollArea" },
            { "ProjectsHeader", "QStackedWidget > QFrame#projectsContent > QFrame" },
            { "ProjectCardOverlay", "QFrame#projectButton > QLabel#labelButton > QLabel#labelButtonOverlay" },
            { "ProjectCardMessage", "QFrame#projectButton > QLabel#labelButton > QWidget > QLabel#projectMessageOverlay" },
            { "ProjectCardSubMessage", "QFrame#projectButton > QLabel#labelButton > QWidget > QLabel#projectSubMessageOverlay" },
            { "ProjectCardOutline", "QFrame#projectsContent > QWidget > QFrame#projectButton > QLabel#labelButton" },
            { "ProjectCardAction",
              "QFrame#projectsContent > QFrame#projectButton > QLabel#labelButton > QWidget > QPushButton#projectActionButton" },
            { "ProjectCardLabel", "QFrame#projectsContent > QFrame#projectButton > QFrame > QLabel#projectNameLabel" },
            { "ProjectCardFooter", "QFrame#projectsContent > QWidget > QFrame#projectButton > QFrame" },
            { "ProjectCard", "QFrame#projectsContent > QWidget#qt_scrollarea_viewport > QWidget > QFrame#projectButton" },
            { "ProjectMenuButton", "QFrame#projectsContent > QFrame#projectButton > QFrame > QPushButton#projectMenuButton" },

            // ---- Form fields
            { "FormDropDownButton",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget#formLineEditWidget > QFrame#formFrame > "
              "QPushButton#dropDownButton" },
            { "FormBrowseButton", "QWidget#formBrowseEditWidget > QFrame#formFrame[Valid=true] > QPushButton" },
            { "FormLineEdit", "QWidget#formLineEditWidget > QFrame#formFrame[Valid=true] > QLineEdit" },
            { "FormComboBox", "QWidget#projectSettingsTab > QWidget#formComboBoxWidget > QFrame#formFrame > QComboBox" },
            { "FormFrameNestedInvalid",
              "QStackedWidget#createAGemRHS > QWidget > QWidget#createAGemRadioButtonSubFormField > QFrame#formFrame[Valid=false]" },
            { "FormFrameNestedFocused",
              "QStackedWidget#createAGemRHS > QWidget > QWidget#createAGemRadioButtonSubFormField > QFrame#formFrame[Focus=true]" },
            { "FormFrameNested",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget > QWidget#createAGemRadioButtonSubFormField > "
              "QFrame#formFrame" },
            { "FormFrameWizardInvalid",
              "QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "FormFrameWizardFocused",
              "QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameWizard",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formFrame" },
            { "FormFrameInvalid", "QMainWindow > QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "FormFrameFocused", "QMainWindow > QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrame", "QFrame#projectSettings > QWidget#formLineEditWidget > QFrame#formFrame[Valid=true]" },
            { "FormOptionsFrameWizard",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget > QWidget#formOptionsWidget > "
              "QFrame#formOptionsFrame" },
            { "FormOptionsCheckBox",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget#formOptionsWidget > QFrame#formOptionsFrame > "
              "QCheckBox" },
            { "FormErrorLabelFlush",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > "
              "QLabel#formErrorLabel" },
            { "FormErrorLabel", "QMainWindow > QWidget#formLineEditWidget > QLabel#formErrorLabel" },
            { "FormRow", "QFrame > QFrame#projectSettings > QWidget#formLineEditWidget" },
            { "FormTagFieldWizard", "QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formTagField" },
            { "FormTagField", "QMainWindow > QWidget#formLineEditWidget > QFrame#formTagField" },
            { "FormTagChip", "QWidget#formLineEditWidget > QFrame#formTagField > QCheckBox" },
            { "FormTitle", "QWidget#qt_scrollarea_viewport > QWidget > QLabel#formTitleLabel" },

            // ---- The create-a-gem wizard
            { "WizardStepRadio", "QFrame#createAGemBody > QFrame > QFrame#createAGemLHS > QRadioButton" },
            { "WizardRightPane", "QFrame#createAGemBody > QFrame > QStackedWidget#createAGemRHS > QScrollArea#createAGemRightPane" },
            { "WizardSubheader",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget#qt_scrollarea_viewport > QWidget > "
              "QLabel#rightPaneSubheader" },
            { "WizardRadioSubtext",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget#qt_scrollarea_viewport > QWidget > "
              "QLabel#createAGemRadioButtonSubtext" },
            { "WizardBody", "QWidget#ScreensCtrl > QStackedWidget > QFrame#createAGemBody" },
            { "WizardFooter", "QStackedWidget > QFrame#createAGemBody > QFrame#createAGemFooter" },

            // ---- Project settings and templates
            { "SettingsTabButton", "QFrame#projectSettingsTopFrame > QWidget#projectSettingsTab > QPushButton[secondary=true]" },
            { "TemplateButton", "QFrame#projectTemplate > QWidget > QWidget > QPushButton#templateButton" },
            { "TemplateImageChecked",
              "QFrame#projectTemplate > QWidget > QPushButton#templateButton[Checked=true] > QLabel#templateImage" },
            { "TemplateImage", "QFrame#projectTemplate > QWidget > QPushButton#templateButton > QLabel#templateImage" },
            { "TemplateLabel", "QFrame#projectTemplate > QWidget > QPushButton#templateButton > QLabel#templateLabel" },
            { "TemplateOverlay", "QPushButton#templateButton > QLabel#templateImage > QLabel#labelButtonOverlay" },
            { "ProjectSettings", "QStackedWidget#body > QFrame > QFrame#projectSettings" },
            { "ProjectTemplate", "QFrame > QFrame#projectSettings > QFrame#projectTemplate" },
            { "TemplateDetailsLabel", "QFrame#projectSettings > QFrame#projectTemplate > QLabel#projectTemplateDetailsLabel" },
            { "TemplateDetails", "QStackedWidget#body > QFrame > QFrame#projectTemplateDetails" },
            { "TemplateSummary", "QFrame > QFrame#projectTemplateDetails > QLabel#summary" },
            { "TemplateGemsTitle", "QFrame > QFrame#projectTemplateDetails > QLabel#includedGemsTitle" },
            { "TemplateMoreGems", "QFrame > QFrame#projectTemplateDetails > QLabel#moreGems" },
            { "TemplateBrowseCatalog", "QFrame > QFrame#projectTemplateDetails > QLabel#browseCatalog" },

            // ---- The gem catalog and the gem repositories
            { "InspectorVersionButton",
              "QScrollArea#GemCatalogInspector > QWidget > QWidget#GemCatalogVersion > QPushButton[secondary=true]" },
            { "InspectorButtonSecondary",
              "QScrollArea#GemCatalogInspector > QWidget#qt_scrollarea_viewport > QWidget > QPushButton[secondary=true]" },
            { "InspectorButtonPrimary", "QScrollArea#GemCatalogInspector > QWidget > QWidget > QPushButton[primary=true]" },
            { "InspectorButtonDanger", "QScrollArea#GemCatalogInspector > QWidget > QWidget > QPushButton[danger=true]" },
            { "RepoInspectorButtonDanger",
              "QScrollArea#gemRepoInspector > QWidget#qt_scrollarea_viewport > QWidget > QPushButton[danger=true]" },
            { "GemRepoAddButton", "QStackedWidget > QFrame > QPushButton#gemRepoAddButton[secondary=true]" },
            { "InspectorNoticeInfo", "QScrollArea#GemCatalogInspector > QWidget > QWidget > QLabel#GemCatalogRequirements" },
            { "InspectorNoticeWarning",
              "QScrollArea#GemCatalogInspector > QWidget > QWidget#GemCatalogVersion > QLabel#GemCatalogCompatibilityWarning" },
            { "InspectorFieldName", "QScrollArea#GemCatalogInspector > QWidget > QWidget#GemCatalogVersion > QLabel[class=label]" },
            { "InspectorTag", "QScrollArea#gemRepoInspector > QWidget > QWidget#TagWidgetContainer > QLabel#TagWidget" },
            { "Tag", "QFrame#projectTemplate > QWidget#includedGems > QLabel#TagWidget" },
            { "InspectorLabel", "QScrollArea#GemCatalogInspector > QWidget#qt_scrollarea_viewport > QWidget > QLabel" },
            { "Inspector", "QFrame#GemCatalogScreen > QStackedWidget > QScrollArea#GemCatalogInspector" },
            { "InspectorTagContainer", "QScrollArea#GemCatalogInspector > QWidget > QWidget > QWidget#TagWidgetContainer" },
            { "RepoInspectorTagContainer", "QScrollArea#gemRepoInspector > QWidget > QWidget > QWidget#TagWidgetContainer" },
            { "FilterCount", "QScrollArea#GemCatalogFilterWidget > QWidget > QWidget > QLabel#CountLabel" },
            { "CartCount", "QFrame#GemCatalogHeaderWidget > QWidget > QLabel#GemCatalogCartCountLabel" },
            { "GemCart", "QFrame#GemCatalogHeaderWidget > QWidget > QScrollArea#GemCatalogCart" },
            { "CartDownloadList", "QWidget > QWidget#GemCatalogCartOverlayGemDownloadHeader > QWidget#GemCatalogCartOverlayGemDownloadBG" },
            { "CartDownloadHeader", "QWidget > QWidget > QWidget#GemCatalogCartOverlayGemDownloadHeader" },
            { "GemList", "QStackedWidget#qt_tabwidget_stackedwidget > QFrame#GemCatalogScreen > QListView#GemCatalogListView" },
            { "GemListCount", "QFrame#GemCatalogScreen > QFrame > QLabel#GemCatalogHeaderShowCountLabel" },
            { "GemListRefreshButton", "QFrame#GemCatalogScreen > QFrame > QPushButton#RefreshButton" },
            { "GemCatalogMenuButton", "QFrame#GemCatalogScreen > QFrame#GemCatalogHeaderWidget > QPushButton#gemCatalogMenuButton" },

            // ---- Dialogs and the toast
            { "ExternalLink", "QMainWindow > QDialog#ExternalLinkDialog > QLabel#externalLink" },
            { "DependencyLabel", "QMainWindow > QDialog#GemDependenciesDialog > QLabel" },
            { "DependencyCheckBox", "QMainWindow > QDialog#GemDependenciesDialog > QCheckBox" },
            { "Toast", "QFrame > QMainWindow > QDialog#ToastNotification" },
            { "ToastIconFrame", "QMainWindow > QDialog#ToastNotification > QFrame#icon_frame" },
            { "ToastIcon", "QDialog#ToastNotification > QFrame#icon_frame > QLabel#iconLabel" },
        };

        // ------------------------------------------------------------------------------
        // Cases a matcher could get wrong: the element a chain gets, or none
        // ------------------------------------------------------------------------------

        const BoxCase BoxTrickyCases[] = {
            // ---- start and projects titles
            { "StartTitle", "QWidget#firstTimeContent > QLabel#titleLabel" },
            { "ProjectsTitle", "QWidget#projectsContent > QFrame > QLabel#titleLabel" },
            // `> QFrame >`: a plain QWidget is no QFrame
            { "", "QWidget#projectsContent > QWidget > QLabel#titleLabel" },
            { "", "QFrame#projectsContent > QWidget > QPushButton#newProjectButton" },
            { "NewProjectButton", "QFrame#projectsContent > QFrame > QPushButton#newProjectButton" },
            { "ProjectsHeader", "QWidget#projectsContent > QLabel#titleLabel" },
            { "", "QWidget#firstTimeContent > QFrame > QLabel#titleLabel" },
            { "", "QWidget > QLabel#titleLabel" },

            // ---- footer roles
            { "FooterButtonPrimary", "QDialog > QDialogButtonBox#footer > QPushButton[primary=true]" },
            { "FooterButtonSecondary", "QDialog > QDialogButtonBox#footer > QPushButton[secondary=true]" },
            { "FooterButtonDanger", "QDialog > QDialogButtonBox#footer > QPushButton[danger=true]" },
            { "", "QDialog > QDialogButtonBox#footer > QPushButton" },
            { "", "QDialog > QDialogButtonBox#footer > QPushButton[primary=false]" },
            { "", "QDialog > QDialogButtonBox > QPushButton[primary=true]" },
            { "", "QDialog > QDialogButtonBox#footer > QWidget > QPushButton[primary=true]" },
            { "", "QDialog > QWidget > QPushButton[secondary=true]" },

            // ---- inspector roles
            { "InspectorButtonPrimary", "QScrollArea#GemCatalogInspector > QWidget > QPushButton[primary=true]" },
            { "InspectorButtonSecondary", "QScrollArea#GemCatalogInspector > QWidget > QPushButton[secondary=true]" },
            { "InspectorButtonDanger", "QScrollArea#GemCatalogInspector > QWidget > QPushButton[danger=true]" },
            { "InspectorVersionButton", "QScrollArea#GemCatalogInspector > QWidget#GemCatalogVersion > QPushButton[secondary=true]" },
            { "", "QScrollArea#GemCatalogInspector > QWidget > QPushButton" },
            { "", "QScrollArea#GemCatalogInspector > QWidget#GemCatalogVersion > QPushButton" },
            { "", "QWidget > QPushButton[primary=true]" },
            { "RepoInspectorButtonDanger", "QScrollArea#gemRepoInspector > QWidget > QPushButton[danger=true]" },
            { "", "QScrollArea#gemRepoInspector > QWidget > QPushButton[secondary=true]" },

            // ---- project card
            { "ProjectCardOutline", "QFrame#projectButton > QLabel#labelButton" },
            { "ProjectCardLabel", "QFrame#projectButton > QFrame > QLabel#labelButton" },
            { "ProjectCardLabel", "QFrame#projectButton > QLabel#labelButton > QLabel" },
            { "ProjectCardAction", "QFrame#projectButton > QLabel#labelButton > QPushButton" },
            { "ProjectCardCancel", "QFrame#projectButton > QLabel#labelButton > QPushButton#projectActionCancelButton" },
            { "", "QFrame > QLabel#labelButton" },
            { "", "QWidget > QPushButton#projectActionCancelButton" },

            // ---- tags
            { "InspectorTag", "QScrollArea#GemCatalogInspector > QWidget > QLabel#TagWidget" },
            { "InspectorTag", "QScrollArea#gemRepoInspector > QWidget > QLabel#TagWidget" },
            { "Tag", "QWidget > QLabel#TagWidget" },
            { "InspectorLabel", "QScrollArea#GemCatalogInspector > QWidget > QLabel" },
            { "", "QWidget > QLabel" },

            // ---- form frames
            { "FormFrame", "QWidget#formLineEditWidget > QFrame#formFrame" },
            { "FormFrame", "QWidget#formLineEditWidget > QFrame#formFrame[Focus=false]" },
            { "FormFrame", "QWidget#formLineEditWidget > QFrame#formFrame[Valid=true]" },
            { "FormFrameFocused", "QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameInvalid", "QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "FormFrameInvalid", "QWidget#formLineEditWidget > QFrame#formFrame[Focus=true][Valid=false]" },
            { "FormFrameWizard", "QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formFrame" },
            { "FormFrameWizardFocused",
              "QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameWizardInvalid",
              "QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "FormFrameNested", "QStackedWidget#createAGemRHS > QWidget > QWidget#createAGemRadioButtonSubFormField > QFrame#formFrame" },
            { "FormFrameNestedInvalid",
              "QStackedWidget#createAGemRHS > QWidget > QWidget#createAGemRadioButtonSubFormField > QFrame#formFrame[Valid=false]" },
            // the nested field's frame is any descendant of it (`#createAGemRadioButtonSubFormField #formFrame`)
            { "FormFrameNested", "QWidget#createAGemRadioButtonSubFormField > QWidget > QFrame#formFrame" },
            { "FormFrameNestedFocused", "QWidget#createAGemRadioButtonSubFormField > QWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameNestedInvalid", "QWidget#createAGemRadioButtonSubFormField > QWidget > QFrame#formFrame[Valid=false]" },
            // the dialogs of the wizard family, each with the three looks; another dialog's frames are the plain ones
            { "FormFrameWizard", "QDialog#addGemRepoDialog > QWidget#formLineEditWidget > QFrame#formFrame" },
            { "FormFrameWizard", "QDialog#addRemoteTemplateDialog > QWidget#formLineEditWidget > QFrame#formFrame" },
            { "FormFrameWizard", "QDialog#addRemoteProjectDialog > QWidget#formLineEditWidget > QFrame#formFrame" },
            { "FormFrameWizardFocused", "QDialog#addGemRepoDialog > QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameWizardFocused", "QDialog#addRemoteTemplateDialog > QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameWizardFocused", "QDialog#addRemoteProjectDialog > QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameWizardInvalid", "QDialog#addGemRepoDialog > QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "FormFrameWizardInvalid", "QDialog#addRemoteTemplateDialog > QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "FormFrameWizardInvalid", "QDialog#addRemoteProjectDialog > QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "FormFrame", "QDialog#addOtherDialog > QWidget#formLineEditWidget > QFrame#formFrame" },
            { "FormFrameFocused", "QDialog#addOtherDialog > QWidget#formLineEditWidget > QFrame#formFrame[Focus=true]" },
            { "FormFrameInvalid", "QDialog#addOtherDialog > QWidget#formLineEditWidget > QFrame#formFrame[Valid=false]" },
            { "", "QWidget#formLineEditWidget > QFrame" },
            { "", "QWidget#formLineEditWidget > QFrame#formFrameOther" },

            // ---- form fields
            { "FormLineEdit", "QWidget#formLineEditWidget > QFrame#formFrame > QLineEdit" },
            { "", "QWidget#formLineEditWidget > QLineEdit" },
            { "FormComboBox", "QWidget#formComboBoxWidget > QFrame#formFrame > QComboBox" },
            { "FormComboBox", "QFrame#formFrame > QWidget > QComboBox" },
            { "", "QWidget#formComboBoxWidget > QFrame > QComboBox" },
            { "", "QWidget > QComboBox" },
            { "FormDropDownButton", "QWidget#formComboBoxWidget > QFrame#formFrame > QPushButton#dropDownButton" },
            { "FormBrowseButton", "QWidget#formBrowseEditWidget > QFrame#formFrame > QPushButton" },
            { "", "QWidget#formBrowseEditWidget > QPushButton" },
            { "FormErrorLabelFlush", "QDialog#addRemoteProjectDialog > QWidget#formLineEditWidget > QLabel#formErrorLabel" },
            { "FormErrorLabel", "QWidget#formLineEditWidget > QLabel#formErrorLabel" },
            { "FormTagChip", "QFrame#formTagField > QCheckBox" },
            { "", "QWidget > QCheckBox" },

            // ---- templates
            { "TemplateImageChecked", "QFrame#projectTemplate > QPushButton#templateButton[Checked=true] > QLabel#templateImage" },
            { "TemplateImage", "QFrame#projectTemplate > QPushButton#templateButton[Checked=false] > QLabel#templateImage" },
            // `#projectTemplate QPushButton[Checked="true"] #templateImage`: the checked button is an ancestor, not only the parent
            { "TemplateImageChecked",
              "QFrame#projectTemplate > QPushButton#templateButton[Checked=true] > QWidget > QLabel#templateImage" },
            { "TemplateImage", "QFrame#projectTemplate > QPushButton#templateButton > QLabel#templateImage" },
            { "", "QWidget > QPushButton#templateButton[Checked=true] > QLabel#templateImage" },
            { "", "QFrame#projectTemplate > QWidget > QLabel#templateImage" },
            { "TemplateLabel", "QFrame#projectTemplate > QPushButton#templateButton > QLabel#templateLabel" },
            { "TemplateButton", "QFrame#projectTemplate > QPushButton#templateButton" },
            { "", "QFrame > QPushButton#templateButton" },
            { "TemplateSummary", "QFrame#projectTemplateDetails > QLabel#summary" },
            { "", "QFrame > QLabel#summary" },

            // ---- the body: `QStackedWidget#createAGemRHS` names its class, `#body` does not
            { "Body", "QFrame > QStackedWidget#createAGemRHS" },
            { "", "QFrame > QFrame#createAGemRHS" },
            { "Body", "QFrame > QFrame#body" },
            { "Body", "QFrame > QStackedWidget#body" },

            // ---- chrome
            { "HeaderBackButton", "QFrame#header > QPushButton" },
            { "HeaderBackButton", "QFrame#header > QWidget > QPushButton" },
            { "", "QWidget > QPushButton" },
            { "Header", "QFrame#header" },
            { "TopBar", "QFrame > QStackedWidget#body > QFrame#engineTopFrame" },
            { "TopBar", "QFrame > QStackedWidget#body > QFrame#projectSettingsTopFrame" },
            { "TopBar", "QFrame#GemCatalogScreen > QFrame#GemCatalogHeaderWidget" },
            { "Footer", "QDialogButtonBox#footer" },
            { "", "QDialogButtonBox" },
            { "WizardStepRadio", "QWidget#createAGemLHS > QRadioButton" },
            { "WizardStepRadio", "QWidget#createAGemLHS > QWidget > QRadioButton" },
            { "", "QWidget > QRadioButton" },
            { "ProjectsGrid", "QWidget#projectsContent > QScrollArea" },
            { "", "QFrame#projectsContent > QFrame > QScrollArea" },
            { "", "QWidget > QScrollArea" },
            { "ProjectsHeader", "QWidget#projectsContent > QFrame" },
            { "", "QWidget#projectsContent > QWidget > QFrame" },
        };

        // ------------------------------------------------------------------------------
        // One case for every text element: the font and colour Native gives its representative widget
        // ------------------------------------------------------------------------------

        const TextCase TextCases[] = {
            // ---- Screen chrome: headers, footers and surfaces
            { "PageTitle", "QFrame > QFrame#header > QLabel#headerSubTitle", 24, 400, false, nullptr, nullptr },
            { "Subtitle", "QFrame > QFrame#header > QLabel#headerTitle", 14, 400, false, nullptr, nullptr },

            // ---- The start page
            { "HeroTitle", "QStackedWidget > QFrame#firstTimeContent > QLabel#titleLabel", 60, 400, false, nullptr, nullptr },
            { "StartCardLabel",
              "QStackedWidget > QFrame#firstTimeContent > QPushButton#createProjectButton",
              14, 400, false, nullptr, nullptr },

            // ---- The projects page and its cards
            { "ProjectCardThisEngine",
              "QFrame#projectsContent > QFrame#projectButton > QFrame > QLabel#thisEngineLabel",
              12, 400, false, "#eeeeee", "#eeeeee" },
            { "ProjectCardOtherEngine",
              "QFrame#projectsContent > QFrame#projectButton > QFrame > QLabel#otherEngineLabel",
              12, 400, false, "#dddddd", "#dddddd" },
            { "ProjectCardMessage",
              "QFrame#projectButton > QLabel#labelButton > QWidget > QLabel#projectMessageOverlay",
              16, 700, false, nullptr, nullptr },
            { "ProjectCardSubMessage",
              "QFrame#projectButton > QLabel#labelButton > QWidget > QLabel#projectSubMessageOverlay",
              12, 700, false, nullptr, nullptr },
            { "ProjectCardLabel",
              "QFrame#projectsContent > QWidget > QFrame#projectButton > QLabel#labelButton",
              14, 700, false, nullptr, nullptr },

            // ---- Form fields
            { "FormLabel", "QWidget#formLineEditWidget > QFrame#formFrame[Valid=true] > QLabel", 13, 400, false, "#cccccc", "#888888" },
            { "FormOptionsLabel",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget#formOptionsWidget > QFrame#formOptionsFrame > "
              "QLabel",
              13, 400, false, "#cccccc", "#cccccc" },
            { "FormErrorLabel", "QMainWindow > QWidget#formLineEditWidget > QLabel#formErrorLabel", 14, 400, false, "#ec3030", "#ec3030" },
            { "PaneHeader",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget#qt_scrollarea_viewport > QWidget > "
              "QLabel#rightPaneHeader",
              21, 400, false, nullptr, nullptr },
            { "FormLineEdit",
              "QWidget#formLineEditWidget > QFrame#formFrame[Valid=true] > QLineEdit",
              18, 400, false, "#ffffff", "#aaaaaa" },
            { "FormComboBox",
              "QWidget#projectSettingsTab > QWidget#formComboBoxWidget > QFrame#formFrame > QComboBox",
              18, 400, false, nullptr, nullptr },
            { "FormOptionsCheckBox",
              "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget#formOptionsWidget > QFrame#formOptionsFrame > "
              "QCheckBox",
              16, 400, false, nullptr, nullptr },
            // PM.qss:239: light blue in every state, the disabled one too (a sheet on the window outranks the application's)
            { "FormTagChip", "QWidget#formLineEditWidget > QFrame#formTagField > QCheckBox", 12, 400, false, "#94d2ff", "#94d2ff" },

            // ---- The create-a-gem wizard
            { "WizardStepRadio",
              "QFrame#createAGemBody > QFrame > QFrame#createAGemLHS > QRadioButton",
              16, 400, false, "#aaaaaa", "#6b6b6b" },

            // ---- Project settings and templates
            { "SmallInfo",
              "QWidget#projectSettingsTab > QStackedWidget#qt_tabwidget_stackedwidget > QFrame > QLabel#projectSmallInfoLabel",
              10, 400, false, nullptr, nullptr },
            { "ProjectTemplateLabel",
              "QFrame#projectSettings > QFrame#projectTemplate > QLabel#projectTemplateLabel",
              16, 100, false, nullptr, nullptr },
            { "TemplateLabelChecked",
              "QFrame#projectTemplate > QWidget > QPushButton#templateButton[Checked=true] > QLabel#templateLabel",
              12, 700, false, nullptr, nullptr },
            { "SettingsTabButton",
              "QFrame#projectSettingsTopFrame > QWidget#projectSettingsTab > QPushButton[secondary=true]",
              13, 400, false, nullptr, nullptr },

            // ---- The gem catalog and the gem repositories
            { "FilterCount",
              "QScrollArea#GemCatalogFilterWidget > QWidget > QWidget > QLabel#CountLabel",
              12, 400, false, "#94d2ff", "#94d2ff" },
            { "FilterCategoryTitle",
              "QScrollArea#GemCatalogFilterWidget > QWidget > QWidget > QLabel#GemCatalogFilterCategoryTitle",
              12, 600, false, nullptr, nullptr },
            { "FilterTagText", "QMainWindow > QFrame > QLabel#FilterTagWidgetTextLabel", 10, 400, false, "#94d2ff", "#94d2ff" },
            { "GemListCount", "QFrame#GemCatalogScreen > QFrame > QLabel#GemCatalogHeaderShowCountLabel", 12, 400, true, nullptr, nullptr },
            { "InspectorNotice",
              "QScrollArea#GemCatalogInspector > QWidget > QWidget > QLabel#GemCatalogRequirements",
              10, 400, false, "#dddddd", "#dddddd" },
            { "InspectorSubText",
              "QScrollArea#GemCatalogInspector > QWidget > QWidget > QLabel#gemSubWidgetTextLabel",
              10, 400, false, "#dddddd", "#dddddd" },
            { "CartSectionLabel", "QWidget > QWidget > QLabel#GemCatalogCartOverlaySectionLabel", 12, 600, false, nullptr, nullptr },
            { "ItemTitle", "QFrame > QFrame#projectTemplateDetails > QLabel#displayName", 18, 400, false, nullptr, nullptr },
            { "SectionTitle", "QStackedWidget > QFrame > QLabel#gemRepoNoReposLabel", 16, 400, false, nullptr, nullptr },
            { "GemRepoAddButton",
              "QStackedWidget > QFrame > QPushButton#gemRepoAddButton[secondary=true]",
              12, 600, false, nullptr, nullptr },

            // ---- Dialogs and the toast
            { "DialogSubtitle", "QMainWindow > QDialog#ExternalLinkDialog > QLabel#dialogSubTitle", 14, 600, false, nullptr, nullptr },
            { "ExternalLink", "QMainWindow > QDialog#ExternalLinkDialog > QLabel#externalLink", 12, 400, false, "#94d2ff", "#94d2ff" },
            { "RemoteInstruction",
              "QMainWindow > QDialog#addRemoteProjectDialog > QLabel#remoteProjectDialogInstructionTitleLabel",
              14, 600, false, nullptr, "#aaaaaa" },
            { "RemoteSection",
              "QMainWindow > QDialog#addRemoteProjectDialog > QLabel#remoteProjectDialogRequirementsTitleLabel",
              16, 300, false, nullptr, "#aaaaaa" },
            { "RemoteLabel",
              "QMainWindow > QDialog#addRemoteProjectDialog > QLabel#remoteProjectDialogRequirementsContentLabel",
              12, 400, false, nullptr, "#aaaaaa" },
            { "RepoInstruction",
              "QMainWindow > QDialog#addGemRepoDialog > QLabel#gemRepoAddDialogInstructionTitleLabel",
              14, 700, false, nullptr, nullptr },
            { "RepoWarning",
              "QMainWindow > QDialog#addGemRepoDialog > QLabel#gemRepoAddDialogWarningLabel",
              12, 400, true, nullptr, nullptr },

            // ---- The tabs
            { "TabBarEngine", "QFrame#engineTopFrame > QWidget#engineTab > QTabBar#engineTabBar", 18, 400, false, nullptr, nullptr },
            { "TabBar", "QStackedWidget > QWidget > QTabBar#qt_tabwidget_tabbar", 24, 400, false, nullptr, nullptr },
        };

        // ------------------------------------------------------------------------------
        // Cases a matcher could get wrong: the font a chain gets, from its element or from none
        // ------------------------------------------------------------------------------

        const TextCase TextTrickyCases[] = {
            // ---- titles
            { "HeroTitle", "QWidget#firstTimeContent > QLabel#titleLabel", 60, 400, false, nullptr, nullptr },
            { "PageTitle", "QWidget#projectsContent > QFrame > QLabel#titleLabel", 24, 400, false, nullptr, nullptr },
            { "", "QWidget > QLabel#titleLabel", 12, 400, false, nullptr, nullptr },
            { "", "QWidget#projectsContent > QLabel#titleLabel", 12, 400, false, nullptr, nullptr },
            { "Subtitle", "QWidget#firstTimeContent > QLabel#introLabel", 14, 400, false, nullptr, nullptr },
            { "", "QWidget > QLabel#introLabel", 12, 400, false, nullptr, nullptr },

            // ---- form labels
            { "FormLabel", "QWidget#formFrame > QLabel", 13, 400, false, "#cccccc", "#888888" },
            { "FormLabel", "QWidget#formFrame > QWidget > QLabel", 13, 400, false, "#cccccc", "#888888" },
            { "FormOptionsLabel", "QWidget#formOptionsFrame > QLabel", 13, 400, false, "#cccccc", "#cccccc" },
            { "", "QWidget > QLabel", 12, 400, false, nullptr, nullptr },
            { "FormErrorLabel", "QWidget#formLineEditWidget > QLabel#formErrorLabel", 14, 400, false, "#ec3030", "#ec3030" },
            { "FormLabel", "QWidget#formFrame > QLabel#formErrorLabel", 13, 400, false, "#cccccc", "#888888" },

            // ---- check boxes
            { "FormTagChip", "QFrame#formTagField > QWidget > QCheckBox", 12, 400, false, "#94d2ff", "#94d2ff" },
            { "", "QWidget > QCheckBox", 12, 400, false, nullptr, nullptr },
            { "", "QFrame#formTagFieldOther > QCheckBox", 12, 400, false, nullptr, nullptr },
            { "FormOptionsCheckBox", "QWidget#formOptionsWidget > QCheckBox", 16, 400, false, nullptr, nullptr },

            // ---- the template list: a checked button is any ancestor of the label (PM.qss:564), inside #projectTemplate
            { "TemplateLabelChecked", "QFrame#projectTemplate > QPushButton#templateButton[Checked=true] > QWidget > QLabel#templateLabel",
              12, 700, false, nullptr, nullptr },
            { "", "QFrame#projectTemplate > QPushButton#templateButton[Checked=false] > QLabel#templateLabel",
              12, 400, false, nullptr, nullptr },
            { "", "QWidget > QPushButton#templateButton[Checked=true] > QLabel#templateLabel", 12, 400, false, nullptr, nullptr },

            // ---- remote project dialog
            { "RemoteLabel", "QDialog#addRemoteProjectDialog > QLabel", 12, 400, false, nullptr, "#aaaaaa" },
            { "", "QDialog#addRemoteProjectDialog > QWidget > QLabel", 12, 400, false, nullptr, nullptr },
            { "RemoteInstruction",
              "QDialog#addRemoteProjectDialog > QLabel#remoteProjectDialogInstructionTitleLabel",
              14, 600, false, nullptr, "#aaaaaa" },

            // ---- labels of a card
            { "ProjectCardLabel", "QFrame#projectButton > QLabel", 14, 700, false, nullptr, nullptr },
            { "ProjectCardThisEngine", "QFrame#projectButton > QLabel#thisEngineLabel", 12, 400, false, "#eeeeee", "#eeeeee" },
            { "", "QWidget > QLabel#thisEngineLabel", 12, 400, false, nullptr, nullptr },
            { "ProjectCardLabel", "QFrame#projectButton > QLabel#thisEngineLabelOther", 14, 700, false, nullptr, nullptr },
            { "ProjectCardMessage",
              "QFrame#projectButton > QLabel#labelButton > QLabel#projectMessageOverlay",
              16, 700, false, nullptr, nullptr },
            { "", "QWidget > QLabel#projectMessageOverlay", 12, 400, false, nullptr, nullptr },

            // ---- buttons
            { "StartCardLabel", "QWidget#firstTimeContent > QPushButton", 14, 400, false, nullptr, nullptr },
            { "", "QWidget#firstTimeContent > QWidget > QPushButton", 12, 400, false, nullptr, nullptr },
            { "SettingsTabButton", "QWidget#projectSettingsTab > QPushButton", 13, 400, false, nullptr, nullptr },
            { "GemRepoAddButton", "QPushButton#gemRepoAddButton", 12, 600, false, nullptr, nullptr },
            { "GemRepoAddButton", "QScrollArea#GemCatalogInspector > QWidget > QPushButton", 12, 600, false, nullptr, nullptr },
            { "", "QWidget > QPushButton", 12, 400, false, nullptr, nullptr },

            // ---- tabs
            { "TabBarEngine", "QWidget > QTabBar#engineTabBar", 18, 400, false, nullptr, nullptr },
            { "TabBar", "QWidget > QTabBar", 24, 400, false, nullptr, nullptr },
            { "TabBar", "QWidget > QTabBar#projectSettingsTabBar", 24, 400, false, nullptr, nullptr },
        };

        // ------------------------------------------------------------------------------
        // The box of every element: what Native's widget of the representative chain has, as the cascade of the legacy sheets gives it
        // (the n2 dumps agree wherever a capture reached the widget; FlatElements.cpp marks the others UNVERIFIED). A box is its parts
        // that are not the default: "margin" and "padding" (left, top, right, bottom), "box" (a margin or a padding of 0 is written),
        // "no-native-border" (a frame, push button, combo box or line edit whose border is the sheet's own), "radius", "size", "min"
        // and "max" (width, height of the contents; a length in em is not carried: OPEN), "align" and "origin" of a background image
        // (left, centre, right and top, centre, bottom), and the qproperty assignments.
        // ------------------------------------------------------------------------------

        struct ElementCase
        {
            const char* m_element;
            const char* m_box;
        };

        const ElementCase ElementCases[] = {
            // ---- The window and the widgets of no particular place
            { "ProgressBar", "box; size -1,5; textVisible=false" },
            { "ScreensCtrl", "min 1200,700" },
            { "MainWindow", "padding 16,25,16,25; align LT; origin content" },

            // ---- Screen chrome: headers, footers and surfaces
            // The footer buttons' maximum height is the code's (PushButton.cpp:107-110): the sheet says 28 (ProjectManager.qss:469), the
            // Native widget has 24 (the n2 dumps), and the element leaves it to the code.
            { "FooterButtonPrimary", "margin 0,0,30,0; no-native-border; radius 3; min 150,28; flat=true" },
            { "FooterButtonSecondary", "margin 0,0,30,0; no-native-border; radius 3; min 150,28; flat=true" },
            { "FooterButtonDanger", "margin 0,0,30,0; no-native-border; radius 3; min 150,28; flat=true" },
            { "HeaderBackButton", "margin 10,20,10,0; min 24,24; max 24,24; align CC; origin content; flat=true; iconSize=50x50" },
            { "HeaderTitle", "box; min -1,20; max -1,20" },
            { "HeaderSubtitle", "padding 0,-5,0,0; min -1,42; max -1,42" },
            { "Header", "min -1,80; max -1,80" },
            { "Footer", "min 50,54; max -1,54" },
            { "Body", "" },
            { "TopBar", "" },

            // ---- The start page
            { "StartCardCreate", "no-native-border; min 210,276; max 210,276; align CC; origin content; flat=true" },
            { "StartCardOpen", "no-native-border; min 210,276; max 210,276; align CC; origin content; flat=true" },
            { "StartCardRemote", "no-native-border; min 210,276; max 210,276; align CC; origin content; flat=true" },
            { "StartTitle", "margin 0,73,0,0; indent=0" },
            { "StartIntro", "margin 0,10,0,60; indent=0" },

            // ---- The projects page and its cards
            { "ProjectCardCancel", "margin 1,0,0,0; no-native-border; radius 3; min 140,24; max 140,24; flat=true" },
            { "NewProjectButton", "min 150,26; max 150,26" },
            { "ProjectsTitle", "margin 0,1,0,1; indent=0" },
            { "ProjectsGrid", "margin 0,40,0,5" },
            { "ProjectsHeader", "margin 0,60,0,0" },
            { "ProjectCardOverlay", "box; indent=0" },
            { "ProjectCardMessage", "box; min 160,32; max 160,32; indent=0" },
            { "ProjectCardSubMessage", "box; min 160,32; max 160,32; indent=0" },
            { "ProjectCardOutline", "box; no-native-border; indent=0" },
            { "ProjectCardAction", "min 140,24; max 140,24" },
            { "ProjectCardLabel", "box; indent=0" },
            { "ProjectCardFooter", "margin 0,8,0,0" },
            { "ProjectCard", "min 210,324; max 210,324" },
            { "ProjectMenuButton", "min 30,14; max 30,14; align CC; flat=true" },

            // ---- Form fields
            { "FormDropDownButton", "margin -10,0,0,0; padding 0,0,0,2; size 35,-1; align CC; flat=true; iconSize=40x40" },
            { "FormBrowseButton", "align CC; flat=true" },
            // `height: 1em` (ProjectManager.qss:305) is the 24px of the entry's 18px Open Sans (the n2 dumps: the entry hints 157x21, the 24px
            // less the padding's -4 above and 1 below)
            { "FormLineEdit", "padding 0,-4,0,1; no-native-border; radius 2; size -1,24" },
            // The combo box of a form field (ProjectManager.qss:149-168 over ComboBox.qss and BaseStyleSheet.qss): 810 wide (the limits of the Native
            // widget, 812, are those and the margin), 26 high, a drop-down of 24 with 20 of padding to its right
            { "FormComboBox", "margin 1,1,1,1; no-native-border; radius 2; size -1,26; min 810,-1; max 810,-1; dropdown 24 0,0,20,0" },
            { "FormFrameNestedInvalid", "margin 23,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrameNestedFocused", "margin 23,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrameNested", "margin 23,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrameWizardInvalid", "margin 0,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrameWizardFocused", "margin 0,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrameWizard", "margin 0,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrameInvalid", "margin 50,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrameFocused", "margin 50,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormFrame", "margin 50,10,0,0; padding 6,0,10,2; no-native-border; radius 4; max 840,-1" },
            { "FormOptionsFrameWizard", "margin 2,10,0,0; padding 6,0,10,2; max 840,-1" },
            { "FormOptionsCheckBox", "size -1,40" },
            { "FormErrorLabelFlush", "margin 0,1,0,1" },
            { "FormErrorLabel", "margin 50,1,0,1" },
            { "FormRow", "max 890,-1" },
            { "FormTagFieldWizard", "padding 6,0,10,2; max 840,-1" },
            { "FormTagField", "margin 50,0,0,0; padding 6,0,10,2; max 840,-1" },
            { "FormTagChip", "padding 7,2,5,2" },
            { "FormTitle", "margin 50,24,0,10" },

            // ---- The create-a-gem wizard
            { "WizardStepRadio", "spacing 10; size -1,40; min 180,-1" },
            { "WizardRightPane", "padding 60,25,0,0; min 660,-1" },
            { "WizardSubheader", "margin 0,1,0,1; padding 0,0,0,20; indent=0" },
            { "WizardRadioSubtext", "margin 0,1,0,1; padding 23,0,0,20" },
            { "WizardBody", "" },
            { "WizardFooter", "" },

            // ---- Project settings and templates
            { "SettingsTabButton", "margin 0,0,30,12; no-native-border; radius 3; min 170,26; max 170,26; flat=true; textalign CC" },
            { "TemplateButton", "min 96,160; max 96,160; flat=true" },
            { "TemplateImageChecked", "margin 0,1,0,1; no-native-border; radius 4" },
            { "TemplateImage", "margin 0,1,0,1; no-native-border; radius 4" },
            { "TemplateLabel", "margin 0,1,0,1; alignment=AlignCenter" },
            { "TemplateOverlay", "margin 0,1,0,1" },
            { "ProjectSettings", "margin 0,30,0,0" },
            { "ProjectTemplate", "margin 50,25,0,0" },
            { "TemplateDetailsLabel", "margin 0,1,0,20; min -1,40" },
            // `max-width: 20%` (ProjectManager.qss:516) is 20px to Qt (qcssparser.cpp:2960-2963), under the 240px minimum and 30px margin:
            // the widget is 270px at least (LimitCases).
            { "TemplateDetails", "margin 30,0,0,0; min 240,-1; max 20,-1" },
            { "TemplateSummary", "margin 0,1,0,1; no-native-border; min -1,80; alignment=AlignTop" },
            { "TemplateGemsTitle", "margin 0,5,0,5" },
            { "TemplateMoreGems", "margin 0,20,0,1" },
            { "TemplateBrowseCatalog", "margin 0,5,0,15" },

            // ---- The gem catalog and the gem repositories
            // The rules of these buttons declare a text-align (ProjectManager.qss:602, :906, :1062, :1123): the sheet lays their label out
            { "InspectorVersionButton", "margin 0,5,0,5; no-native-border; radius 3; min -1,24; max -1,24; flat=true; textalign CC" },
            { "InspectorButtonSecondary", "no-native-border; radius 3; min -1,24; max -1,24; flat=true; textalign CC" },
            { "InspectorButtonPrimary", "no-native-border; radius 3; min -1,24; max -1,24; flat=true; textalign CC" },
            { "InspectorButtonDanger", "no-native-border; radius 3; min -1,24; max -1,24; flat=true; textalign CC" },
            { "RepoInspectorButtonDanger", "margin 0,10,0,0; no-native-border; radius 3; min -1,24; max -1,24; flat=true; textalign CC" },
            { "GemRepoAddButton", "no-native-border; radius 3; min 120,24; max 120,24; flat=true; textalign CC" },
            { "InspectorNoticeInfo", "margin 0,0,0,10; padding 34,0,0,0; min -1,34; align LC" },
            { "InspectorNoticeWarning", "margin 0,0,0,10; padding 34,0,0,0; min -1,34; align LC" },
            { "InspectorFieldName", "box; min 50,-1; max 50,-1" },
            { "InspectorTag", "padding 3,3,3,3; no-native-border; radius 3" },
            { "Tag", "margin 0,1,0,1; padding 3,3,3,3; no-native-border; radius 3" },
            { "InspectorLabel", "box" },
            { "Inspector", "box" },
            { "InspectorTagContainer", "min 210,-1; max 210,-1" },
            { "RepoInspectorTagContainer", "box; min 210,-1; max 210,-1" },
            { "FilterCount", "margin 0,1,0,1; no-native-border; radius 3" },
            { "CartCount", "margin 0,1,0,1; no-native-border; radius 3" },
            { "GemCart", "box" },
            { "CartDownloadList", "box" },
            { "CartDownloadHeader", "box" },
            { "GemList", "box; no-native-border" },
            { "GemListCount", "margin 0,1,0,1" },
            { "GemListRefreshButton", "min 24,24; max 24,24; align CC; flat=true" },
            { "GemCatalogMenuButton", "min 36,24; max 36,24; flat=true" },

            // ---- Dialogs and the toast
            { "ExternalLink", "margin 0,5,0,30" },
            { "DependencyLabel", "margin 0,1,0,10" },
            { "DependencyCheckBox", "margin 0,5,5,0; padding 4,4,4,4; radius 3" },
            { "Toast", "radius 20; minimumSize=100x50" },
            { "ToastIconFrame", "no-native-border; radius 4; minimumSize=44x20" },
            { "ToastIcon", "margin 6,1,0,1; maximumSize=30x20; minimumSize=30x20" },
        };

        // ------------------------------------------------------------------------------
        // The faces: what an element draws in a state, as the cascade of the legacy sheets gives it for the state (the winning rule of
        // each property, the later rule over the earlier of equal specificity). The states that hold together are the ones a widget can
        // be in: a key press is a press with no pointer over the button, and the order of the entries of the table of a face is only
        // right when the last entry whose states hold is that rule. A face is its parts that are set: the fill ("solid", or the stops of
        // a vertical "gradient"), the solid edges (the sides, top right bottom left, that share a width and a colour), the label colour
        // of the state (push buttons and radio buttons) and the background image. An element has a row for its idle face and for each
        // state that draws otherwise; the focus together with a hover or a press is a row only where the element has a focus face. The
        // hover, pressed and focus box of the form combo box are OPEN (not carried).
        // ------------------------------------------------------------------------------

        struct StateCase
        {
            const char* m_name;
            QStyle::State m_state;
        };

        const StateCase States[] = {
            { "idle", QStyle::State_Enabled },
            { "hover", QStyle::State_Enabled | QStyle::State_MouseOver },
            { "pressed", QStyle::State_Enabled | QStyle::State_MouseOver | QStyle::State_Sunken },
            { "pressedAlone", QStyle::State_Enabled | QStyle::State_Sunken },
            { "focus", QStyle::State_Enabled | QStyle::State_HasFocus },
            { "hoverFocus", QStyle::State_Enabled | QStyle::State_MouseOver | QStyle::State_HasFocus },
            { "pressedFocus", QStyle::State_Enabled | QStyle::State_Sunken | QStyle::State_HasFocus },
            { "checked", QStyle::State_Enabled | QStyle::State_On },
            { "disabled", QStyle::State_None },
        };

        struct FaceCase
        {
            const char* m_element;
            const char* m_state;
            const char* m_face;
        };

        const FaceCase FaceCases[] = {
            // ---- The window and the widgets of no particular place
            { "ProgressBar", "idle", "fill solid #00000000" },
            { "ScreensCtrl", "idle", "fill solid #00000000" },
            { "MainWindow", "idle", "fill solid #ff131313; image :/o3de.svg" },

            // ---- Screen chrome: headers, footers and surfaces
            { "FooterButtonPrimary", "idle", "fill gradient #ff0095f2 #ff1e70eb" },
            { "FooterButtonPrimary", "hover", "fill gradient #ff10a5f2 #ff2e80eb" },
            { "FooterButtonPrimary", "pressed", "fill gradient #ff0085e2 #ff0e60db" },
            { "FooterButtonPrimary", "pressedAlone", "fill gradient #ff0085e2 #ff0e60db" },
            { "FooterButtonPrimary", "disabled", "fill gradient #ff777777 #ff444444" },
            { "FooterButtonSecondary", "idle", "fill gradient #ff888888 #ff555555" },
            { "FooterButtonSecondary", "hover", "fill gradient #ff999999 #ff666666" },
            { "FooterButtonSecondary", "pressed", "fill gradient #ff555555 #ff777777" },
            { "FooterButtonSecondary", "pressedAlone", "fill gradient #ff555555 #ff777777" },
            { "FooterButtonDanger", "idle", "fill gradient #ffe32c27 #ff951d21" },
            { "FooterButtonDanger", "hover", "fill gradient #fffd3129 #ffaf2221" },
            { "FooterButtonDanger", "pressed", "fill gradient #ff951d1f #ffc92724" },
            { "FooterButtonDanger", "pressedAlone", "fill gradient #ff951d1f #ffc92724" },
            { "HeaderBackButton", "idle", "fill solid #00000000; image :/ArrowBack.svg" },
            { "HeaderBackButton", "hover", "fill solid #00000000; image :/ArrowBack_Hover.svg" },
            { "HeaderBackButton", "pressed", "fill solid #00000000; image :/ArrowBack_Hover.svg" },
            { "HeaderBackButton", "pressedAlone", "fill solid #00000000; image :/ArrowBack_Hover.svg" },
            { "HeaderTitle", "idle", "fill solid #00000000" },
            { "HeaderSubtitle", "idle", "fill solid #00000000" },
            { "Header", "idle", "fill solid #ff111111" },
            { "Footer", "idle", "fill solid #00000000" },
            { "Body", "idle", "fill solid #ff333333" },
            { "TopBar", "idle", "fill solid #ff1e252f" },

            // ---- The start page
            { "StartCardCreate", "idle", "fill solid #b4000000; edges trbl 1 #ffffffff; image :/AddOffset.svg" },
            { "StartCardCreate", "hover", "fill solid #b4000000; edges trbl 1 #ff1e70eb; text #ff1e70eb; image :/AddOffset_Hover.svg" },
            { "StartCardCreate", "pressed", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/AddOffset_Hover.svg" },
            { "StartCardCreate",
              "pressedAlone", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/AddOffset_Hover.svg" },
            { "StartCardCreate", "focus", "fill solid #b4000000; edges trbl 1 #ff1e70eb; image :/AddOffset.svg" },
            { "StartCardCreate",
              "hoverFocus", "fill solid #b4000000; edges trbl 1 #ff1e70eb; text #ff1e70eb; image :/AddOffset_Hover.svg" },
            { "StartCardCreate",
              "pressedFocus", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/AddOffset_Hover.svg" },
            { "StartCardOpen", "idle", "fill solid #b4000000; edges trbl 1 #ffffffff; image :/FolderOffset.svg" },
            { "StartCardOpen", "hover", "fill solid #b4000000; edges trbl 1 #ff1e70eb; text #ff1e70eb; image :/FolderOffset_Hover.svg" },
            { "StartCardOpen", "pressed", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/FolderOffset_Hover.svg" },
            { "StartCardOpen",
              "pressedAlone", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/FolderOffset_Hover.svg" },
            { "StartCardOpen", "focus", "fill solid #b4000000; edges trbl 1 #ff1e70eb; image :/FolderOffset.svg" },
            { "StartCardOpen",
              "hoverFocus", "fill solid #b4000000; edges trbl 1 #ff1e70eb; text #ff1e70eb; image :/FolderOffset_Hover.svg" },
            { "StartCardOpen",
              "pressedFocus", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/FolderOffset_Hover.svg" },
            { "StartCardRemote", "idle", "fill solid #b4000000; edges trbl 1 #ffffffff; image :/Cloud.svg" },
            { "StartCardRemote", "hover", "fill solid #b4000000; edges trbl 1 #ff1e70eb; text #ff1e70eb; image :/Cloud_Hover.svg" },
            { "StartCardRemote", "pressed", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/Cloud_Hover.svg" },
            { "StartCardRemote", "pressedAlone", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/Cloud_Hover.svg" },
            { "StartCardRemote", "focus", "fill solid #b4000000; edges trbl 1 #ff1e70eb; image :/Cloud.svg" },
            { "StartCardRemote", "hoverFocus", "fill solid #b4000000; edges trbl 1 #ff1e70eb; text #ff1e70eb; image :/Cloud_Hover.svg" },
            { "StartCardRemote", "pressedFocus", "fill solid #b4000000; edges trbl 1 #ff0e60eb; text #ff0e60eb; image :/Cloud_Hover.svg" },
            { "StartTitle", "idle", "fill solid #00000000" },
            { "StartIntro", "idle", "fill solid #00000000" },

            // ---- The projects page and its cards
            { "ProjectCardCancel", "idle", "fill gradient #ffe32c27 #ff951d21" },
            { "ProjectCardCancel", "hover", "fill gradient #fffd3129 #ffaf2221" },
            { "ProjectCardCancel", "pressed", "fill gradient #ff951d1f #ffc92724" },
            { "ProjectCardCancel", "pressedAlone", "fill gradient #ff951d1f #ffc92724" },
            { "NewProjectButton", "idle", "fill solid #00000000" },
            { "ProjectsTitle", "idle", "fill solid #00000000" },
            { "ProjectsGrid", "idle", "fill solid #00000000" },
            { "ProjectsHeader", "idle", "fill solid #00000000" },
            { "ProjectCardOverlay", "idle", "fill solid #b2000000" },
            { "ProjectCardMessage", "idle", "fill solid #00000000" },
            { "ProjectCardSubMessage", "idle", "fill solid #00000000" },
            { "ProjectCardOutline", "idle", "fill solid #00000000; edges trbl 1 #ffffffff" },
            { "ProjectCardOutline", "hover", "fill solid #00000000; edges trbl 1 #ff1e70eb" },
            { "ProjectCardOutline", "pressed", "fill solid #00000000; edges trbl 1 #ff1e70eb" },
            { "ProjectCardOutline", "pressedAlone", "fill solid #00000000; edges trbl 1 #ff1e70eb" },
            { "ProjectCardOutline", "focus", "fill solid #00000000; edges trbl 1 #ff1e70eb" },
            { "ProjectCardOutline", "hoverFocus", "fill solid #00000000; edges trbl 1 #ff1e70eb" },
            { "ProjectCardOutline", "pressedFocus", "fill solid #00000000; edges trbl 1 #ff1e70eb" },
            { "ProjectCardAction", "idle", "fill solid #00000000" },
            { "ProjectCardLabel", "idle", "fill solid #00000000" },
            { "ProjectCardFooter", "idle", "fill solid #00000000" },
            { "ProjectCard", "idle", "fill solid #00000000" },
            { "ProjectMenuButton", "idle", "fill solid #00000000; image :/menu.svg" },

            // ---- Form fields
            { "FormDropDownButton", "idle", "image :/CarrotArrowDown.svg" },
            { "FormBrowseButton", "idle", "fill solid #00000000; image :/Select_Folder.svg" },
            { "FormBrowseButton", "disabled", "fill solid #00000000; image :/Select_Folder_Disabled.svg" },
            { "FormLineEdit", "idle", "fill solid #00000000" },
            // The faces of the form combo box (ComboBox.qss:19, :22-42, :53-57): each state that moves the box gives its own margin and radius.
            // A press with no hover is the idle face (no rule), a hover with the focus the focus face (:31 comes after :22), an open popup
            // (Checked) the focus face (:32), a disabled one the fill and colour of :53 and, with the focus, the box of :31
            { "FormComboBox", "idle", "fill solid #ffcccccc; text #ff000000" },
            { "FormComboBox", "hover", "fill solid #ffcccccc; edges trbl 1 #32000000; text #ff000000; margin 0,0,0,0; radius 3" },
            { "FormComboBox", "pressed", "fill solid #ffcccccc; edges trbl 1 #32000000; text #ff000000; margin 0,0,0,0; radius 3" },
            { "FormComboBox", "pressedAlone", "fill solid #ffcccccc; text #ff000000" },
            { "FormComboBox", "focus", "fill solid #ffffffff; edges trbl 1 #ff94d2ff; text #ff000000; margin 0,0,0,0; radius 3" },
            { "FormComboBox", "hoverFocus", "fill solid #ffffffff; edges trbl 1 #ff94d2ff; text #ff000000; margin 0,0,0,0; radius 3" },
            { "FormComboBox", "pressedFocus", "fill solid #ffffffff; edges trbl 1 #ff94d2ff; text #ff000000; margin 0,0,0,0; radius 3" },
            { "FormComboBox", "checked", "fill solid #ffffffff; edges trbl 1 #ff94d2ff; text #ff000000; margin 0,0,0,0; radius 3" },
            { "FormComboBox", "disabled", "fill solid #ff666666; text #ff999999" },
            { "FormFrameNestedInvalid", "idle", "fill solid #ff444444; edges trbl 1 #ffff0000" },
            { "FormFrameNestedInvalid", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrameNestedFocused", "idle", "fill solid #ff444444; edges trbl 1 #ff1e70eb" },
            { "FormFrameNestedFocused", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrameNested", "idle", "fill solid #ff444444; edges trbl 1 #ffdddddd" },
            { "FormFrameNested", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrameWizardInvalid", "idle", "fill solid #ff444444; edges trbl 1 #ffff0000" },
            { "FormFrameWizardInvalid", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrameWizardFocused", "idle", "fill solid #ff444444; edges trbl 1 #ff1e70eb" },
            { "FormFrameWizardFocused", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrameWizard", "idle", "fill solid #ff444444; edges trbl 1 #ffdddddd" },
            { "FormFrameWizard", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrameInvalid", "idle", "fill solid #ff444444; edges trbl 1 #ffff0000" },
            { "FormFrameInvalid", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrameFocused", "idle", "fill solid #ff444444; edges trbl 1 #ff1e70eb" },
            { "FormFrameFocused", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormFrame", "idle", "fill solid #ff444444; edges trbl 1 #ffdddddd" },
            { "FormFrame", "disabled", "fill solid #ff444444; edges trbl 1 #ff959595" },
            { "FormOptionsFrameWizard", "idle", "fill solid #00000000" },
            { "FormOptionsCheckBox", "idle", "fill solid #00000000" },
            { "FormErrorLabelFlush", "idle", "fill solid #00000000" },
            { "FormErrorLabel", "idle", "fill solid #00000000" },
            { "FormRow", "idle", "fill solid #00000000" },
            { "FormTagFieldWizard", "idle", "fill solid #ff333333" },
            { "FormTagField", "idle", "fill solid #ff333333" },
            { "FormTagChip", "idle", "fill solid #ff555555" },
            { "FormTitle", "idle", "fill solid #00000000" },

            // ---- The create-a-gem wizard
            { "WizardStepRadio", "idle", "fill solid #00000000" },
            { "WizardStepRadio", "checked", "fill solid #ff333333; text #ffffffff; spacing 30" },
            { "WizardRightPane", "idle", "fill solid #00000000" },
            { "WizardSubheader", "idle", "fill solid #00000000" },
            { "WizardRadioSubtext", "idle", "fill solid #00000000" },
            { "WizardBody", "idle", "fill solid #ff444444" },
            { "WizardFooter", "idle", "fill solid #ff000000" },

            // ---- Project settings and templates
            { "SettingsTabButton", "idle", "fill gradient #ff888888 #ff555555" },
            { "SettingsTabButton", "hover", "fill gradient #ff999999 #ff666666" },
            { "SettingsTabButton", "pressed", "fill gradient #ff555555 #ff777777" },
            { "SettingsTabButton", "pressedAlone", "fill gradient #ff555555 #ff777777" },
            { "TemplateButton", "idle", "fill solid #00000000" },
            { "TemplateButton", "hover", "fill solid #ff444444" },
            { "TemplateButton", "pressed", "fill solid #ff444444" },
            { "TemplateImageChecked", "idle", "fill solid #00000000; edges trbl 3 #ff1e70eb" },
            { "TemplateImage", "idle", "fill solid #00000000; edges trbl 3 #00000000" },
            { "TemplateLabel", "idle", "fill solid #00000000" },
            { "TemplateOverlay", "idle", "fill solid #b2000000" },
            { "ProjectSettings", "idle", "fill solid #00000000" },
            { "ProjectTemplate", "idle", "fill solid #00000000" },
            { "TemplateDetailsLabel", "idle", "fill solid #00000000" },
            { "TemplateDetails", "idle", "fill solid #ff444444" },
            { "TemplateSummary", "idle", "fill solid #00000000; edges b 2 #ff555555" },
            { "TemplateGemsTitle", "idle", "fill solid #00000000" },
            { "TemplateMoreGems", "idle", "fill solid #00000000" },
            { "TemplateBrowseCatalog", "idle", "fill solid #00000000" },

            // ---- The gem catalog and the gem repositories
            { "InspectorVersionButton", "idle", "fill gradient #ff888888 #ff555555" },
            { "InspectorVersionButton", "hover", "fill gradient #ff999999 #ff666666" },
            { "InspectorVersionButton", "pressed", "fill gradient #ff555555 #ff777777" },
            { "InspectorVersionButton", "pressedAlone", "fill gradient #ff555555 #ff777777" },
            { "InspectorButtonSecondary", "idle", "fill gradient #ff888888 #ff555555" },
            { "InspectorButtonSecondary", "hover", "fill gradient #ff999999 #ff666666" },
            { "InspectorButtonSecondary", "pressed", "fill gradient #ff555555 #ff777777" },
            { "InspectorButtonSecondary", "pressedAlone", "fill gradient #ff555555 #ff777777" },
            { "InspectorButtonPrimary", "idle", "fill gradient #ff0095f2 #ff1e70eb" },
            { "InspectorButtonPrimary", "hover", "fill gradient #ff10a5f2 #ff2e80eb" },
            { "InspectorButtonPrimary", "pressed", "fill gradient #ff0085e2 #ff0e60db" },
            { "InspectorButtonPrimary", "pressedAlone", "fill gradient #ff0085e2 #ff0e60db" },
            { "InspectorButtonPrimary", "disabled", "fill gradient #ff777777 #ff444444" },
            { "InspectorButtonDanger", "idle", "fill gradient #ffe32c27 #ff951d21" },
            { "InspectorButtonDanger", "hover", "fill gradient #fffd3129 #ffaf2221" },
            { "InspectorButtonDanger", "pressed", "fill gradient #ff951d1f #ffc92724" },
            { "InspectorButtonDanger", "pressedAlone", "fill gradient #ff951d1f #ffc92724" },
            { "RepoInspectorButtonDanger", "idle", "fill gradient #ffe32c27 #ff951d21" },
            { "RepoInspectorButtonDanger", "hover", "fill gradient #fffd3129 #ffaf2221" },
            { "RepoInspectorButtonDanger", "pressed", "fill gradient #ff951d1f #ffc92724" },
            { "RepoInspectorButtonDanger", "pressedAlone", "fill gradient #ff951d1f #ffc92724" },
            { "GemRepoAddButton", "idle", "fill gradient #ff888888 #ff555555" },
            { "GemRepoAddButton", "hover", "fill gradient #ff999999 #ff666666" },
            { "GemRepoAddButton", "pressed", "fill gradient #ff555555 #ff777777" },
            { "GemRepoAddButton", "pressedAlone", "fill gradient #ff555555 #ff777777" },
            { "InspectorNoticeInfo", "idle", "fill solid #00000000; image :/Info.svg" },
            { "InspectorNoticeWarning", "idle", "fill solid #00000000; image :/Warning.svg" },
            { "InspectorFieldName", "idle", "fill solid #00000000" },
            { "InspectorTag", "idle", "fill solid #ff333333" },
            { "Tag", "idle", "fill solid #ff333333" },
            { "InspectorLabel", "idle", "fill solid #00000000" },
            { "Inspector", "idle", "fill solid #ff444444" },
            { "InspectorTagContainer", "idle", "fill solid #00000000" },
            { "RepoInspectorTagContainer", "idle", "fill solid #00000000" },
            { "FilterCount", "idle", "fill solid #ff333333" },
            { "CartCount", "idle", "fill solid #ff4285f4" },
            { "GemCart", "idle", "fill solid #ff555555" },
            { "CartDownloadList", "idle", "fill solid #ff444444" },
            { "CartDownloadHeader", "idle", "fill solid #ff333333" },
            { "GemList", "idle", "fill solid #ff333333" },
            // The fill of these two is the inline sheet of GemListHeaderWidget (GemListHeaderWidget.cpp:28), `background-color: #333333`
            // for its whole subtree, which the sheets' cascade does not know.
            { "GemListCount", "idle", "fill solid #ff333333" },
            { "GemListRefreshButton", "idle", "fill solid #ff333333; image :/Refresh.svg" },
            { "GemListRefreshButton", "hover", "fill solid #ff333333; image :/Refresh_Hover.svg" },
            { "GemListRefreshButton", "pressed", "fill solid #ff333333; image :/Refresh_Hover.svg" },
            { "GemCatalogMenuButton", "idle", "fill solid #00000000" },

            // ---- Dialogs and the toast
            { "ExternalLink", "idle", "fill solid #00000000" },
            { "DependencyLabel", "idle", "fill solid #00000000" },
            { "DependencyCheckBox", "idle", "fill solid #ff333333" },
            { "Toast", "idle", "fill solid #ff000000; edges trbl 1 #ffdddddd" },
            { "ToastIconFrame", "idle", "fill solid #00000000" },
            { "ToastIcon", "idle", "fill solid #00000000" },
        };

        // ------------------------------------------------------------------------------
        // The contents of the frames: the offsets of the Native widget's contents rect from its edges (left, top, right, bottom; the n2
        // dumps), which are its margin, border and padding in one. A frame whose box is Qt's own is not here: the element leaves it.
        // ------------------------------------------------------------------------------

        struct ContentsCase
        {
            const char* m_element;
            int m_left;
            int m_top;
            int m_right;
            int m_bottom;
        };

        const ContentsCase ContentsCases[] = {
            // ---- Screen chrome: headers, footers and surfaces
            { "HeaderTitle", 0, 0, 0, 0 },
            { "HeaderSubtitle", 0, -5, 0, 0 },

            // ---- The start page
            { "StartTitle", 0, 73, 0, 0 },
            { "StartIntro", 0, 10, 0, 60 },

            // ---- The projects page and its cards
            { "ProjectsTitle", 0, 1, 0, 1 },
            { "ProjectsGrid", 1, 41, 1, 6 },
            { "ProjectsHeader", 0, 60, 0, 0 },
            { "ProjectCardOutline", 1, 1, 1, 1 },
            { "ProjectCardLabel", 0, 0, 0, 0 },
            { "ProjectCardFooter", 0, 8, 0, 0 },

            // ---- Form fields
            { "FormFrameNested", 30, 11, 11, 3 },
            { "FormFrameWizard", 7, 11, 11, 3 },
            { "FormFrame", 57, 11, 11, 3 },
            { "FormOptionsFrameWizard", 8, 10, 10, 2 },
            { "FormTitle", 50, 24, 0, 10 },

            // ---- The create-a-gem wizard
            { "WizardRightPane", 61, 26, 1, 1 },
            { "WizardSubheader", 0, 1, 0, 21 },
            { "WizardRadioSubtext", 23, 1, 0, 21 },

            // ---- Project settings and templates
            { "ProjectSettings", 0, 30, 0, 0 },
            { "ProjectTemplate", 50, 25, 0, 0 },
            { "TemplateDetailsLabel", 0, 1, 0, 20 },
            { "TemplateDetails", 30, 0, 0, 0 },
            { "TemplateSummary", 0, 1, 0, 3 },
            { "TemplateGemsTitle", 0, 5, 0, 5 },
            { "TemplateMoreGems", 0, 20, 0, 1 },
            { "TemplateBrowseCatalog", 0, 5, 0, 15 },

            // ---- The gem catalog and the gem repositories
            { "InspectorFieldName", 0, 0, 0, 0 },
            { "InspectorTag", 3, 3, 3, 3 },
            { "InspectorLabel", 0, 0, 0, 0 },
            { "Inspector", 1, 1, 1, 1 },
            { "FilterCount", 0, 1, 0, 1 },
            { "CartCount", 0, 1, 0, 1 },
            { "GemList", 0, 0, 0, 0 },
            { "GemListCount", 0, 1, 0, 1 },
        };

        // ------------------------------------------------------------------------------
        // The limits of a widget after its element is applied: the sizes the Native widget has (the n2 dumps; the sheet's min/max plus
        // the box it grows by: a push button has the style's frame, 1px on each side, until it is drawn flat, qstylesheetstyle.cpp:2572).
        // 0 and -1: none. What the code sets (PushButton::polish gives every push button a maximum height, PushButton.cpp:107-110;
        // ProjectButtonWidget.cpp fixes the size of a project card's face) is not the element's: a footer button's maximum height
        // is the code's, so none here.
        // ------------------------------------------------------------------------------

        struct LimitCase
        {
            const char* m_chain;
            int m_minWidth;
            int m_minHeight;
            int m_maxWidth;
            int m_maxHeight;
        };

        const LimitCase LimitCases[] = {
            { "QFrame > QMainWindow > QWidget#ScreensCtrl", 1200, 700, -1, -1 },
            { "QFrame#projectsContent > QFrame > QPushButton#newProjectButton", 152, 28, 152, 28 },
            { "QFrame#projectsContent > QWidget > QFrame#projectButton", 210, 324, 210, 324 },
            { "QFrame#projectsContent > QFrame#projectButton > QLabel#labelButton > QWidget > QPushButton#projectActionButton",
              142, 26, 142, 26 },
            { "QFrame#projectsContent > QFrame#projectButton > QFrame > QPushButton#projectMenuButton", 32, 16, 32, 16 },
            { "QStackedWidget > QFrame > QFrame#header", 0, 80, -1, 80 },
            { "QFrame > QFrame#header > QPushButton", 46, 46, 46, 46 },
            { "QFrame > QFrame#header > QLabel#headerTitle", 0, 20, -1, 20 },
            { "QFrame > QFrame#header > QLabel#headerSubTitle", 0, 37, -1, 37 },
            { "QStackedWidget > QFrame > QDialogButtonBox#footer", 50, 54, -1, 54 },
            { "QFrame > QDialogButtonBox#footer > QPushButton[primary=true]", 180, 28, -1, -1 },
            { "QStackedWidget > QFrame#firstTimeContent > QPushButton#createProjectButton", 212, 278, 212, 278 },
            { "QStackedWidget > QFrame#firstTimeContent > QPushButton#addProjectButton", 212, 278, 212, 278 },
            { "QStackedWidget > QFrame#firstTimeContent > QPushButton#addRemoteProjectButton", 212, 278, 212, 278 },
            { "QFrame > QFrame#projectSettings > QWidget#formLineEditWidget", 0, 0, 890, -1 },
            { "QFrame#projectSettings > QWidget#formLineEditWidget > QFrame#formFrame[Valid=true]", 0, 0, 908, -1 },
            { "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget > QWidget#formLineEditWidget > QFrame#formFrame",
              0, 0, 858, -1 },
            { "QStackedWidget#createAGemRHS > QWidget > QWidget#createAGemRadioButtonSubFormField > QFrame#formFrame", 0, 0, 881, -1 },
            { "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QWidget > QWidget#formOptionsWidget > QFrame#formOptionsFrame",
              0, 0, 858, -1 },
            { "QFrame#createAGemBody > QFrame > QFrame#createAGemLHS > QRadioButton", 180, 0, -1, -1 },
            // n2: the combo box of a form field has min [812, 0] and max [812, -1] (810 and the 1px margin on both sides)
            { "QWidget#projectSettingsTab > QWidget#formComboBoxWidget > QFrame#formFrame > QComboBox", 812, 0, 812, -1 },
            // n2: the entry of a form field has none (its height is the contents', not a limit)
            { "QWidget#formLineEditWidget > QFrame#formFrame[Valid=true] > QLineEdit", 0, 0, -1, -1 },
            { "QFrame#createAGemBody > QFrame > QStackedWidget#createAGemRHS > QScrollArea#createAGemRightPane", 722, 0, -1, -1 },
            { "QFrame#projectSettings > QFrame#projectTemplate > QLabel#projectTemplateDetailsLabel", 0, 61, -1, -1 },
            { "QStackedWidget#body > QFrame > QFrame#projectTemplateDetails", 270, 0, 50, -1 },
            { "QFrame > QFrame#projectTemplateDetails > QLabel#summary", 0, 84, -1, -1 },
            { "QFrame#projectSettingsTopFrame > QWidget#projectSettingsTab > QPushButton[secondary=true]", 200, 38, 200, 38 },
            { "QStackedWidget > QFrame > QPushButton#gemRepoAddButton[secondary=true]", 120, 24, 120, 24 },
            { "QScrollArea#gemRepoInspector > QWidget#qt_scrollarea_viewport > QWidget > QPushButton[danger=true]", 0, 34, -1, 34 },
            { "QScrollArea#GemCatalogInspector > QWidget#qt_scrollarea_viewport > QWidget > QPushButton[secondary=true]", 0, 24, -1, 24 },
            { "QScrollArea#GemCatalogInspector > QWidget > QWidget#GemCatalogVersion > QLabel[class=label]", 50, 0, 50, -1 },
            { "QScrollArea#GemCatalogInspector > QWidget > QWidget > QWidget#TagWidgetContainer", 210, 0, 210, -1 },
            { "QFrame#GemCatalogScreen > QFrame#GemCatalogHeaderWidget > QPushButton#gemCatalogMenuButton", 38, 26, 38, 26 },
            { "QFrame#GemCatalogScreen > QFrame > QPushButton#RefreshButton", 26, 26, 26, 26 },
        };

        QStyle::State StateNamed(const char* name)
        {
            for (const StateCase& stateCase : States)
            {
                if (QByteArray(stateCase.m_name) == name)
                {
                    return stateCase.m_state;
                }
            }
            ADD_FAILURE() << "no state named " << name;
            return QStyle::State_None;
        }

        //! The chain a Look is tested with: that of its box case ("QWidget" where there is none, which the case reports)
        const char* ChainOf(const char* look)
        {
            for (const BoxCase& boxCase : BoxCases)
            {
                if (QByteArray(boxCase.m_element) == look)
                {
                    return boxCase.m_chain;
                }
            }
            ADD_FAILURE() << "no box case for " << look;
            return "QWidget";
        }

        //! The widgets of the chain of a Look, the last polished as the flattened style polishes it: it has the element of the Look
        class ElementOfLook
        {
        public:
            explicit ElementOfLook(const char* look)
                : m_widgets(ChainOf(look))
            {
                AzQtComponents::BoxElement::apply(m_widgets.Leaf());
                m_element = AzQtComponents::BoxElement::forWidget(m_widgets.Leaf());
            }

            const AzQtComponents::BoxElement* Element() const
            {
                return m_element;
            }

            const QWidget* Widget() const
            {
                return m_widgets.Leaf();
            }

        private:
            WidgetChain m_widgets;
            const AzQtComponents::BoxElement* m_element = nullptr;
        };
    } // namespace FlatElementsTestsInternal

    // ----------------------------------------------------------------------------------
    // Fixture: an application with the fonts and the style it has in the flattened GUI, and the elements registered
    // ----------------------------------------------------------------------------------

    class FlatElementsTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        FlatElementsTests()
            : m_application(m_argc, m_argv)
        {
            // The application font (StyleManager.cpp:220-222), then the class fonts the flattened GUI installs from its style
            QFont applicationFont(QStringLiteral("Open Sans"));
            applicationFont.setPixelSize(12);
            QApplication::setFont(applicationFont);

            // As StyleManager::initialize builds it (StyleManager.cpp:244-246); the application owns it
            QApplication::setStyle(new AzQtComponents::Style(QStyleFactory::create(QStringLiteral("Fusion"))));
            AzQtComponents::TextElement::installClassFonts();

            RegisterFlatElements();
        }

        //! The Look of the box element the last widget of a chain gets when the style polishes it; "": none
        static QByteArray BoxLookOf(const char* chain)
        {
            FlatElementsTestsInternal::WidgetChain widgets(chain);
            AzQtComponents::BoxElement::apply(widgets.Leaf());
            const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(widgets.Leaf());
            return element ? QByteArray(element->m_name).mid(sizeof("ProjectManager.Box.") - 1) : QByteArray();
        }

        static QByteArray Hex(const QColor& color)
        {
            return color.name(QColor::HexArgb).toLatin1();
        }

        // ------------------------------------------------------------------------------
        // The element's numbers as the tables write them
        // ------------------------------------------------------------------------------

        //! "solid #aarrggbb", or "gradient" and the colour of each stop, or nothing
        static QByteArray FillText(const QBrush& brush)
        {
            switch (brush.style())
            {
            case Qt::NoBrush:
                return QByteArray();
            case Qt::SolidPattern:
                return "solid " + Hex(brush.color());
            case Qt::LinearGradientPattern:
                {
                    QByteArray text = "gradient";
                    for (const QGradientStop& stop : brush.gradient()->stops())
                    {
                        text += " " + Hex(stop.second);
                    }
                    return text;
                }
            default:
                return "other";
            }
        }

        //! The solid edges: the sides that share a width and a colour, "trbl 1 #ffffffff" for a border, "b 2 #ff555555" for a line under
        static QByteArray EdgesText(const AzQtComponents::BoxElement::Edges& edges)
        {
            const AzQtComponents::BoxElement::Edge* sides[] = { &edges.m_top, &edges.m_right, &edges.m_bottom, &edges.m_left };
            const char letters[] = "trbl";
            const auto drawn = [&sides](int side) { return sides[side]->m_width > 0 && sides[side]->m_color.isValid(); };
            bool grouped[4] = {};
            QByteArrayList groups;
            for (int side = 0; side < 4; ++side)
            {
                if (grouped[side] || !drawn(side))
                {
                    continue;
                }
                QByteArray group;
                for (int other = side; other < 4; ++other)
                {
                    if (!grouped[other] && drawn(other) && sides[other]->m_width == sides[side]->m_width &&
                        sides[other]->m_color == sides[side]->m_color)
                    {
                        grouped[other] = true;
                        group += letters[other];
                    }
                }
                groups << group + " " + QByteArray::number(sides[side]->m_width) + " " + Hex(sides[side]->m_color);
            }
            return groups.join("; ");
        }

        //! What a face sets: "fill ...; edges ...; text #aarrggbb; image :/resource.svg"
        static QByteArray FaceText(const AzQtComponents::BoxElement::Face& face)
        {
            QByteArrayList parts;
            if (const QByteArray fill = FillText(face.m_fill); !fill.isEmpty())
            {
                parts << "fill " + fill;
            }
            if (const QByteArray edges = EdgesText(face.m_edges); !edges.isEmpty())
            {
                parts << "edges " + edges;
            }
            if (face.m_text.isValid())
            {
                parts << "text " + Hex(face.m_text);
            }
            if (face.m_image[0])
            {
                parts << QByteArray("image ") + face.m_image;
            }
            if (face.m_margin.has_value())
            {
                const QMargins& m = *face.m_margin;
                parts << "margin " + QByteArray::number(m.left()) + "," + QByteArray::number(m.top()) + "," + QByteArray::number(m.right()) + "," +
                    QByteArray::number(m.bottom());
            }
            if (face.m_radius != AzQtComponents::BoxElement::Unset)
            {
                parts << "radius " + QByteArray::number(face.m_radius);
            }
            if (face.m_spacing != AzQtComponents::BoxElement::Unset)
            {
                parts << "spacing " + QByteArray::number(face.m_spacing);
            }
            return parts.join("; ");
        }

        //! "LT": where a background image sits, left/centre/right and top/centre/bottom
        static QByteArray AlignText(Qt::Alignment align)
        {
            const char horizontal = align.testFlag(Qt::AlignRight) ? 'R' : (align.testFlag(Qt::AlignHCenter) ? 'C' : 'L');
            const char vertical = align.testFlag(Qt::AlignBottom) ? 'B' : (align.testFlag(Qt::AlignVCenter) ? 'C' : 'T');
            return QByteArray(1, horizontal) + QByteArray(1, vertical);
        }

        //! A qproperty value: "true", "50x50", or its text
        static QByteArray ValueText(const QVariant& value)
        {
            switch (value.typeId())
            {
            case QMetaType::Bool:
                return value.toBool() ? "true" : "false";
            case QMetaType::QSize:
                return QByteArray::number(value.toSize().width()) + "x" + QByteArray::number(value.toSize().height());
            default:
                return value.toString().toUtf8();
            }
        }

        //! The box of an element, its parts that are not the default, in the order of the box table's description
        static QByteArray BoxText(const AzQtComponents::BoxElement& element, const QWidget* widget)
        {
            const auto sides = [](const QMargins& m)
            {
                return QByteArray::number(m.left()) + "," + QByteArray::number(m.top()) + "," + QByteArray::number(m.right()) + "," +
                    QByteArray::number(m.bottom());
            };
            const auto pair = [](const QSize& size) { return QByteArray::number(size.width()) + "," + QByteArray::number(size.height()); };
            const auto isSet = [](const QSize& size) { return size.width() >= 0 || size.height() >= 0; };

            QByteArrayList parts;
            if (!element.m_margin.isNull())
            {
                parts << "margin " + sides(element.m_margin);
            }
            if (!element.m_padding.isNull())
            {
                parts << "padding " + sides(element.m_padding);
            }
            if (element.m_hasBox && element.m_margin.isNull() && element.m_padding.isNull())
            {
                parts << "box";
            }
            // a widget whose border the style can draw: its flag is carried (the other classes have no border unless a rule gives one)
            const bool canHaveNativeBorder = qobject_cast<const QFrame*>(widget) || qobject_cast<const QPushButton*>(widget) ||
                qobject_cast<const QComboBox*>(widget) || qobject_cast<const QLineEdit*>(widget);
            if (!element.m_nativeBorder && canHaveNativeBorder)
            {
                parts << "no-native-border";
            }
            if (element.m_radius)
            {
                parts << "radius " + QByteArray::number(element.m_radius);
            }
            if (element.m_spacing != AzQtComponents::BoxElement::Unset)
            {
                parts << "spacing " + QByteArray::number(element.m_spacing);
            }
            for (const auto& [name, size] : { std::pair<const char*, QSize>{ "size ", element.m_size },
                                              std::pair<const char*, QSize>{ "min ", element.m_minSize },
                                              std::pair<const char*, QSize>{ "max ", element.m_maxSize } })
            {
                if (isSet(size))
                {
                    parts << name + pair(size);
                }
            }
            if (element.m_dropDownWidth != AzQtComponents::BoxElement::Unset || !element.m_dropDownPadding.isNull())
            {
                parts << "dropdown " + QByteArray::number(element.m_dropDownWidth) + " " + sides(element.m_dropDownPadding);
            }
            bool hasImage = element.m_idle.m_image[0] != 0;
            for (const AzQtComponents::BoxElement::StateFace& stateFace : element.m_stateFaces)
            {
                hasImage = hasImage || (stateFace.m_states != 0 && stateFace.m_face.m_image[0] != 0);
            }
            if (hasImage)
            {
                parts << "align " + AlignText(element.m_imageAlign);
                if (element.m_imageOrigin == AzQtComponents::BoxElement::Origin::Content)
                {
                    parts << "origin content";
                }
            }
            QByteArrayList properties;
            for (const AzQtComponents::BoxElement::Property& property : element.m_properties)
            {
                if (property.m_name)
                {
                    properties << QByteArray(property.m_name) + "=" + ValueText(property.m_value);
                }
            }
            std::sort(properties.begin(), properties.end());
            parts += properties;
            if (element.m_textAlign)
            {
                parts << "textalign " + AlignText(element.m_textAlign);
            }
            return parts.join("; ");
        }

        //! The face the element of a Look draws in a state, against what the cascade gives that state
        static void ExpectFace(const FlatElementsTestsInternal::FaceCase& faceCase)
        {
            const FlatElementsTestsInternal::ElementOfLook resolved(faceCase.m_element);
            if (!resolved.Element())
            {
                ADD_FAILURE() << "no element for " << faceCase.m_element;
                return;
            }
            QStyleOption option;
            option.state = FlatElementsTestsInternal::StateNamed(faceCase.m_state);
            EXPECT_STREQ(FaceText(resolved.Element()->face(&option)).constData(), faceCase.m_face)
                << faceCase.m_state << ": " << faceCase.m_element;
        }

        static void ExpectBoxLook(const FlatElementsTestsInternal::BoxCase& boxCase)
        {
            EXPECT_STREQ(BoxLookOf(boxCase.m_chain).constData(), boxCase.m_element) << boxCase.m_chain;
        }

        //! The font and the colours the text element of a chain's last widget gives it, as the style polishes it
        static void ExpectText(const FlatElementsTestsInternal::TextCase& textCase)
        {
            FlatElementsTestsInternal::WidgetChain widgets(textCase.m_chain);
            QWidget* widget = widgets.Leaf();
            AzQtComponents::TextElement::apply(widget);
            AzQtComponents::TextElement::applyColor(widget);

            const QFont font = widget->font();
            EXPECT_EQ(font.pixelSize(), textCase.m_pixelSize) << textCase.m_element << ": " << textCase.m_chain;
            EXPECT_EQ(font.weight(), textCase.m_weight) << textCase.m_element << ": " << textCase.m_chain;
            EXPECT_EQ(font.italic(), textCase.m_italic) << textCase.m_element << ": " << textCase.m_chain;
            if (textCase.m_color)
            {
                const QColor color = widget->palette().color(QPalette::Active, QPalette::WindowText);
                EXPECT_STREQ(Hex(color).constData(), Hex(QColor(textCase.m_color)).constData())
                    << textCase.m_element << ": " << textCase.m_chain;
            }
            if (textCase.m_disabledColor)
            {
                const QColor color = widget->palette().color(QPalette::Disabled, QPalette::WindowText);
                EXPECT_STREQ(Hex(color).constData(), Hex(QColor(textCase.m_disabledColor)).constData())
                    << textCase.m_element << " (disabled): " << textCase.m_chain;
            }
        }

    protected:
        int m_argc = 1;                                     // QApplication keeps a reference to it
        char m_programName[5] = "test";
        char* m_argv[2] = { m_programName, nullptr };
        QApplication m_application;
    };

    // ----------------------------------------------------------------------------------
    // Boxes
    // ----------------------------------------------------------------------------------

    TEST_F(FlatElementsTests, BoxElements_EveryElementIsTheOneOfItsNativeWidget)
    {
        for (const FlatElementsTestsInternal::BoxCase& boxCase : FlatElementsTestsInternal::BoxCases)
        {
            ExpectBoxLook(boxCase);
        }
    }

    TEST_F(FlatElementsTests, BoxElements_ATrickyMatcherSelectsOnlyItsWidgets)
    {
        for (const FlatElementsTestsInternal::BoxCase& boxCase : FlatElementsTestsInternal::BoxTrickyCases)
        {
            ExpectBoxLook(boxCase);
        }
    }

    TEST_F(FlatElementsTests, BoxElements_EveryElementCarriesTheBoxOfItsNativeWidget)
    {
        for (const FlatElementsTestsInternal::ElementCase& elementCase : FlatElementsTestsInternal::ElementCases)
        {
            const FlatElementsTestsInternal::ElementOfLook resolved(elementCase.m_element);
            if (!resolved.Element())
            {
                ADD_FAILURE() << "no element for " << elementCase.m_element;
                continue;
            }
            EXPECT_STREQ(BoxText(*resolved.Element(), resolved.Widget()).constData(), elementCase.m_box) << elementCase.m_element;
        }
    }

    TEST_F(FlatElementsTests, BoxElements_EveryStateDrawsTheFaceOfItsCascade)
    {
        for (const FlatElementsTestsInternal::FaceCase& faceCase : FlatElementsTestsInternal::FaceCases)
        {
            ExpectFace(faceCase);
        }
    }

    TEST_F(FlatElementsTests, BoxElements_ContentsAreTheOffsetsOfTheNativeFrame)
    {
        for (const FlatElementsTestsInternal::ContentsCase& contentsCase : FlatElementsTestsInternal::ContentsCases)
        {
            const FlatElementsTestsInternal::ElementOfLook resolved(contentsCase.m_element);
            if (!resolved.Element())
            {
                ADD_FAILURE() << "no element for " << contentsCase.m_element;
                continue;
            }
            // The rect of the frame is far larger than its box: the contents' offsets are the box's, whatever the size
            QStyleOptionFrame option;
            option.state = QStyle::State_Enabled;
            option.rect = QRect(0, 0, 1000, 1000);
            EXPECT_TRUE(resolved.Element()->ownsBox(&option)) << contentsCase.m_element;

            const QRect contents = resolved.Element()->contentsRect(&option, resolved.Widget());
            EXPECT_EQ(contents.left(), contentsCase.m_left) << contentsCase.m_element;
            EXPECT_EQ(contents.top(), contentsCase.m_top) << contentsCase.m_element;
            EXPECT_EQ(option.rect.right() - contents.right(), contentsCase.m_right) << contentsCase.m_element;
            EXPECT_EQ(option.rect.bottom() - contents.bottom(), contentsCase.m_bottom) << contentsCase.m_element;
        }
    }

    TEST_F(FlatElementsTests, BoxElements_LimitsAreTheSizesOfTheNativeWidget)
    {
        const auto none = [](int value) { return value >= QWIDGETSIZE_MAX ? -1 : value; };
        for (const FlatElementsTestsInternal::LimitCase& limitCase : FlatElementsTestsInternal::LimitCases)
        {
            FlatElementsTestsInternal::WidgetChain widgets(limitCase.m_chain);
            QWidget* widget = widgets.Leaf();
            AzQtComponents::BoxElement::apply(widget);

            EXPECT_EQ(widget->minimumWidth(), limitCase.m_minWidth) << limitCase.m_chain;
            EXPECT_EQ(widget->minimumHeight(), limitCase.m_minHeight) << limitCase.m_chain;
            EXPECT_EQ(none(widget->maximumWidth()), limitCase.m_maxWidth) << limitCase.m_chain;
            EXPECT_EQ(none(widget->maximumHeight()), limitCase.m_maxHeight) << limitCase.m_chain;
        }
    }

    // ----------------------------------------------------------------------------------
    // Text
    // ----------------------------------------------------------------------------------

    TEST_F(FlatElementsTests, TextElements_EveryElementGivesItsNativeWidgetTheNativeFontAndColour)
    {
        for (const FlatElementsTestsInternal::TextCase& textCase : FlatElementsTestsInternal::TextCases)
        {
            ExpectText(textCase);
        }
    }

    TEST_F(FlatElementsTests, TextElements_ATrickyMatcherSelectsOnlyItsWidgets)
    {
        for (const FlatElementsTestsInternal::TextCase& textCase : FlatElementsTestsInternal::TextTrickyCases)
        {
            ExpectText(textCase);
        }
    }

    // ----------------------------------------------------------------------------------
    // Tabs
    // ----------------------------------------------------------------------------------

    TEST_F(FlatElementsTests, TabElements_EveryBarResolvesToItsLook)
    {
        struct TabCase
        {
            const char* m_look;
            const char* m_chain;
        };
        const TabCase cases[] = {
            { "Engine", "QWidget > QTabBar#engineTabBar" },
            { "Settings", "QWidget > QTabBar#projectSettingsTabBar" },
            { "Header", "QWidget > QTabBar" },
            { "Header", "QWidget > QTabBar#qt_tabwidget_tabbar" },
        };
        for (const TabCase& tabCase : cases)
        {
            FlatElementsTestsInternal::WidgetChain widgets(tabCase.m_chain);
            const AzQtComponents::TabElement* element = AzQtComponents::TabElement::resolve(widgets.Leaf());
            ASSERT_NE(element, nullptr) << tabCase.m_chain;
            EXPECT_STREQ(element->m_name, (QByteArray("ProjectManager.Tab.") + tabCase.m_look).constData()) << tabCase.m_chain;
            EXPECT_EQ(AzQtComponents::TabElement::forTabBar(widgets.Leaf()), element) << tabCase.m_chain;
        }
    }

    TEST_F(FlatElementsTests, TabElements_ValuesAreTheRulesOfTheirNativeBar)
    {
        // The sub-controls of ProjectManager.qss:61-143, 585-591 and 1011-1034 (QTabBar::tab, QTabWidget::tab-bar and ::pane), for each
        // state a tab can be in: idle, hover, selected, pressed and focused (Qt presses a tab as it selects it, and only the current
        // tab has the focus). The text of a face is its rule's colour; a weight of 0 is the bar's own.
        struct TabValues
        {
            const char* m_chain;
            int m_barLeft;
            int m_height;
            int m_minWidth;
            const char* m_idleText;
            int m_idleWeight;
            const char* m_hoverText;
            int m_hoverWeight;
            const char* m_selectedText;
            const char* m_underline;
            int m_selectedWeight;
            const char* m_pressedText;
            const char* m_focusText;                // "": no focus face, the engine's tabs show the selected one
        };
        const TabValues cases[] = {
            { "QWidget > QTabBar#qt_tabwidget_tabbar", 78, 82, 100, "#ffffffff", 0, "#ff1e70eb", 500, "#ff1e70eb", "#ff1e70eb", 500,
              "#ff0e60eb", "#ff4082eb" },
            { "QWidget > QTabBar#projectSettingsTabBar", 60, 50, 100, "#ffffffff", 0, "#ff1e70eb", 500, "#ff1e70eb", "#ff1e70eb", 500,
              "#ff0e60eb", "#ff4082eb" },
            { "QWidget > QTabBar#engineTabBar", 60, 50, 160, "#ffffffff", 400, "#ff94d2ff", 600, "#ff94d2ff", "#ff94d2ff", 600,
              "#ff66bcfa", "" },
        };
        for (const TabValues& values : cases)
        {
            FlatElementsTestsInternal::WidgetChain widgets(values.m_chain);
            const AzQtComponents::TabElement* element = AzQtComponents::TabElement::resolve(widgets.Leaf());
            ASSERT_NE(element, nullptr) << values.m_chain;

            EXPECT_EQ(element->m_barLeft, values.m_barLeft) << values.m_chain;
            EXPECT_EQ(element->m_contentHeight, values.m_height) << values.m_chain;
            EXPECT_EQ(element->m_minContentWidth, values.m_minWidth) << values.m_chain;
            EXPECT_EQ(element->m_maxContentWidth, 200) << values.m_chain;                           // TabWidget.qss:18
            EXPECT_EQ(element->m_margin, QMargins(0, 0, 40, 0)) << values.m_chain;                  // PM.qss:72
            EXPECT_STREQ(Hex(element->m_paneFill).constData(), "#ff333333") << values.m_chain;      // PM.qss:119
            EXPECT_TRUE(element->m_hideFocusRect) << values.m_chain;                                // PM.qss:135-139

            EXPECT_STREQ(Hex(element->m_idle.m_text).constData(), values.m_idleText) << values.m_chain;
            EXPECT_EQ(element->m_idle.m_weight, values.m_idleWeight) << values.m_chain;
            EXPECT_STREQ(Hex(element->m_hover.m_text).constData(), values.m_hoverText) << values.m_chain;
            EXPECT_EQ(element->m_hover.m_weight, values.m_hoverWeight) << values.m_chain;
            EXPECT_STREQ(Hex(element->m_selected.m_text).constData(), values.m_selectedText) << values.m_chain;
            EXPECT_STREQ(Hex(element->m_selectedHover.m_text).constData(), values.m_selectedText) << values.m_chain;
            EXPECT_EQ(element->m_selected.m_weight, values.m_selectedWeight) << values.m_chain;
            EXPECT_STREQ(Hex(element->m_pressed.m_text).constData(), values.m_pressedText) << values.m_chain;
            EXPECT_EQ(element->m_focus.isSet(), values.m_focusText[0] != 0) << values.m_chain;
            if (values.m_focusText[0])
            {
                EXPECT_STREQ(Hex(element->m_focus.m_text).constData(), values.m_focusText) << values.m_chain;
                EXPECT_STREQ(Hex(element->m_focus.m_fill).constData(), "#ff525252") << values.m_chain;      // PM.qss:141
            }

            // The underline is 3px of the tab's colour when it is selected or pressed, and 3px clear otherwise (PM.qss:73, 124)
            for (const AzQtComponents::TabElement::TabFace* face : { &element->m_idle, &element->m_hover })
            {
                EXPECT_EQ(face->m_edges.m_bottom.m_width, 3) << values.m_chain;
                EXPECT_EQ(face->m_edges.m_bottom.m_color.alpha(), 0) << values.m_chain;
            }
            for (const AzQtComponents::TabElement::TabFace* face : { &element->m_selected, &element->m_selectedHover, &element->m_pressed })
            {
                EXPECT_EQ(face->m_edges.m_bottom.m_width, 3) << values.m_chain;
                EXPECT_STREQ(Hex(face->m_edges.m_bottom.m_color).constData(), values.m_underline) << values.m_chain;
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Frames
    // ----------------------------------------------------------------------------------

    TEST_F(FlatElementsTests, FrameElements_TheSeparatorsResolveToTheirLookAndAnyOtherFrameDoesNot)
    {
        struct FrameCase
        {
            const char* m_name;
            QFrame::Shape m_shape;
            const char* m_element;                          // "": an element that is not this module's
            const char* m_foreground;                       // the line's colour (ProjectManager.qss:482, 486)
        };
        const FrameCase cases[] = {
            { "horizontalSeparatingLine", QFrame::HLine, "ProjectManager.Frame.HorizontalSeparator", "#ff666666" },
            { "verticalSeparatingLine", QFrame::VLine, "ProjectManager.Frame.VerticalSeparator", "#ff888888" },
            { "horizontalSeparatingLineOther", QFrame::HLine, "", "" },
            { "", QFrame::HLine, "", "" },
        };
        for (const FrameCase& frameCase : cases)
        {
            QFrame frame;
            frame.setObjectName(QString::fromLatin1(frameCase.m_name));
            frame.setFrameShape(frameCase.m_shape);
            const AzQtComponents::FrameElement* element = AzQtComponents::FrameElement::resolve(&frame);
            ASSERT_NE(element, nullptr) << frameCase.m_name;
            if (frameCase.m_element[0])
            {
                EXPECT_STREQ(element->m_name, frameCase.m_element) << frameCase.m_name;
                EXPECT_STREQ(Hex(element->m_foreground).constData(), frameCase.m_foreground) << frameCase.m_name;
                // the fill every divider has (BaseStyleSheet.qss:41-48), on the shapes the sheet's list names
                EXPECT_STREQ(Hex(element->m_fill).constData(), "#ff444444") << frameCase.m_name;
                EXPECT_EQ(element->m_fillShapes, AzQtComponents::FrameElement::PanelShapes) << frameCase.m_name;
            }
            else
            {
                EXPECT_FALSE(QByteArray(element->m_name).startsWith("ProjectManager.")) << frameCase.m_name;
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // Indicators: the radio buttons of the wizard's steps have the box of a radio button with nothing in it (ProjectManager.qss:97-99)
    // ----------------------------------------------------------------------------------

    TEST_F(FlatElementsTests, IndicatorElements_TheStepsOfTheWizardHaveTheBoxOfARadioButtonWithNothingInIt)
    {
        using AzQtComponents::IndicatorElement;

        // The steps captured in n2 (the create gem page: QFrame#createAGemLHS > QRadioButton, three of them) and every radio button below the list
        for (const char* chain : { "QFrame#createAGemBody > QFrame > QFrame#createAGemLHS > QRadioButton",
                                   "QWidget#createAGemLHS > QWidget > QRadioButton" })
        {
            FlatElementsTestsInternal::WidgetChain widgets(chain);
            const IndicatorElement* element = IndicatorElement::forWidget(widgets.Leaf());
            ASSERT_NE(element, nullptr) << chain;
            EXPECT_STREQ(element->m_name, "ProjectManager.Indicator.WizardStep") << chain;
            EXPECT_TRUE(element->m_hidden) << chain;

            // RadioButton.qss:24-26, :31-33: 16px with a margin of 2px, 20px of box (the size of every radio button's), 18px with 1px of margin while focused
            EXPECT_EQ(element->boxSize(), QSize(20, 20)) << chain;
            EXPECT_EQ(element->m_imageSize, QSize(16, 16)) << chain;
            EXPECT_EQ(element->m_margin, 2) << chain;
            EXPECT_EQ(element->m_focusImageSize, QSize(18, 18)) << chain;
            EXPECT_EQ(element->m_focusMargin, 1) << chain;
        }

        // The radio buttons of the right pane (QRadioButton#createAGem, the template options) keep their mark, and a check box is no radio button
        for (const char* chain : { "QFrame#createAGemBody > QStackedWidget#createAGemRHS > QRadioButton#createAGem",
                                   "QWidget > QRadioButton",
                                   "QWidget#createAGemLHSOther > QRadioButton" })
        {
            FlatElementsTestsInternal::WidgetChain widgets(chain);
            const IndicatorElement* element = IndicatorElement::forWidget(widgets.Leaf());
            ASSERT_NE(element, nullptr) << chain;
            EXPECT_FALSE(element->m_hidden) << chain;
            EXPECT_FALSE(QByteArray(element->m_name).startsWith("ProjectManager.")) << chain;
        }
        FlatElementsTestsInternal::WidgetChain check("QWidget#createAGemLHS > QCheckBox");
        const IndicatorElement* checkElement = IndicatorElement::forWidget(check.Leaf());
        ASSERT_NE(checkElement, nullptr);
        EXPECT_FALSE(checkElement->m_hidden);
    }

    // ----------------------------------------------------------------------------------
    // Item views: `#adjustableHeaderWidget QHeaderView::section { background-color: transparent }` (ProjectManager.qss:872-874)
    // ----------------------------------------------------------------------------------

    TEST_F(FlatElementsTests, ItemViewElements_TheHeaderOfTheGemListIsClearAndEveryOtherTableKeepsTheGlobalSection)
    {
        using AzQtComponents::ItemViewElement;

        // The header of the gem list and of the repositories table (n2: the viewport of its header is the page's #333333 on the Gems and
        // the Remote Sources pages; f5, with the global section, #2d2d2d)
        FlatElementsTestsInternal::WidgetChain widgets("QStackedWidget > QFrame > QTableWidget#adjustableHeaderWidget");
        auto* table = qobject_cast<QTableWidget*>(widgets.Leaf());
        ASSERT_NE(table, nullptr);
        table->setColumnCount(3);
        ItemViewElement::apply(table);
        const ItemViewElement* element = ItemViewElement::forWidget(table);
        ASSERT_NE(element, nullptr);
        EXPECT_STREQ(element->m_name, "ProjectManager.ItemView.AdjustableHeader");
        EXPECT_EQ(element->m_headerFill.alpha(), 0);
        EXPECT_EQ(ItemViewElement::headerFillOf(table->horizontalHeader()).alpha(), 0);

        // What the module's rule does not say is the global element's (TableView.qss): the roles and the faces of a table
        const ItemViewElement defaults;
        EXPECT_EQ(element->m_alternate, defaults.m_alternate);
        EXPECT_EQ(element->m_selection, defaults.m_selection);
        EXPECT_EQ(element->m_itemHeight, 24);
        EXPECT_EQ(element->m_itemPadding, QMargins(4, 0, 0, 0));
        EXPECT_TRUE(element->fillOf(ItemViewElement::Selected).isValid());

        // A table of another name, or any other view, has the section of TableView.qss:69, #2d2d2d
        for (const char* chain : { "QStackedWidget > QFrame > QTableWidget#otherTable", "QStackedWidget > QFrame > QTableWidget" })
        {
            FlatElementsTestsInternal::WidgetChain other(chain);
            auto* otherTable = qobject_cast<QTableWidget*>(other.Leaf());
            ASSERT_NE(otherTable, nullptr) << chain;
            otherTable->setColumnCount(3);
            ItemViewElement::apply(otherTable);
            const ItemViewElement* otherElement = ItemViewElement::forWidget(otherTable);
            ASSERT_NE(otherElement, nullptr) << chain;
            EXPECT_STREQ(otherElement->m_name, "Global.ItemView.Table") << chain;
            EXPECT_STREQ(Hex(ItemViewElement::headerFillOf(otherTable->horizontalHeader())).constData(), "#ff2d2d2d") << chain;
        }
    }

    // ----------------------------------------------------------------------------------
    // The entry of a form: `height: 1em` (ProjectManager.qss:305) is the height of its font
    // ----------------------------------------------------------------------------------

    TEST_F(FlatElementsTests, FormLineEdit_TheHeightOfAnEmIsTheHeightOfItsFontAndTheTwentyFourOfTheElement)
    {
        // An em is QFontMetrics::height() of the font of the rule (qcssparser.cpp:462-467), here the 18px Open Sans of :301: what the element
        // carries (24, as the n2 dumps hold it: the entry hints 21 = 24 - 4 + 1 in every form) holds for that font. Without it another font has
        // another em, and what is compared is not the rule's.
        // The font of the application is the one the library carries (StyleManager::initializeFonts)
        const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/AzQtFonts/Fonts/Open_Sans/OpenSans-Regular.ttf"));
        QFont font(QStringLiteral("Open Sans"));
        font.setPixelSize(18);
        if (fontId < 0 || QFontInfo(font).family() != QLatin1String("Open Sans"))
        {
            GTEST_SKIP() << "Open Sans is not available: an em of the entry is the height of another font";
        }

        // The cascade of the legacy sheets for `#formFrame QLineEdit`: LineEdit.qss:11-19, BaseStyleSheet.qss:68-82, ProjectManager.qss:299-307
        QFrame legacyFrame;
        legacyFrame.setObjectName(QStringLiteral("formFrame"));
        auto* legacyEdit = new QLineEdit(&legacyFrame);
        legacyFrame.setStyleSheet(QStringLiteral(
            "QLineEdit { height: 16px; padding-left: 0px; border-width: 2px; border-style: solid; padding-bottom: 1px; padding-top: -1px; }"
            "QLineEdit { background-color: #CCCCCC; color: black; font-family: \"Open Sans\"; border-radius: 2px; border-width: 0px;"
            " border-color: #e9e9e9; font-size: 12px; line-height: 16px; }"
            "#formFrame QLineEdit { background-color: rgba(0,0,0,0); font-size: 18px; color: #ffffff; border: 0; line-height: 30px;"
            " height: 1em; padding-top: -4px; }"));
        legacyFrame.setAttribute(Qt::WA_DontShowOnScreen);
        legacyFrame.show();
        QApplication::processEvents();
        const int em = QFontMetrics(legacyEdit->font()).height();
        EXPECT_EQ(legacyEdit->font().pixelSize(), 18);

        FlatElementsTestsInternal::WidgetChain widgets("QFrame#formFrame > QLineEdit");
        QWidget* flatEdit = widgets.Leaf();
        AzQtComponents::BoxElement::apply(flatEdit);
        const AzQtComponents::BoxElement* element = AzQtComponents::BoxElement::forWidget(flatEdit);
        ASSERT_NE(element, nullptr);
        EXPECT_STREQ(element->m_name, "ProjectManager.Box.FormLineEdit");

        // 24 for the font of n2, and the hint of the entry the legacy sheet sized: its contents (the em), the padding above (-4px) and below (1px).
        // The height is no limit of the widget (the sheet's `height` is the size of its contents, not a minimum or a maximum)
        EXPECT_EQ(em, 24);
        EXPECT_EQ(element->m_size.height(), em);
        EXPECT_EQ(legacyEdit->sizeHint().height(), em + element->m_padding.top() + element->m_padding.bottom());
        EXPECT_EQ(legacyEdit->sizeHint().height(), 21);
        EXPECT_EQ(flatEdit->minimumHeight(), legacyEdit->minimumHeight());
        EXPECT_EQ(flatEdit->maximumHeight(), legacyEdit->maximumHeight());
    }
} // namespace O3DE::ProjectManager

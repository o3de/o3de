/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/FieldTwins.h>

#include <AzQtComponents/Components/Widgets/ComboBox.h>

#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <vector>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // A combo box (ComboBox.qss, BaseStyleSheet.qss:68-81): the flattened Style against the legacy sheet. The sheet boxed a combo box by
    // its own rules, whatever QComboBox::frame said, and changed the margin, the border and the radius of the box with the state: the
    // size, the rects of the sub-controls (the frame, the edit field of an editable combo box, the drop-down whose width the error
    // button of a combo box keeps clear of) and the pixels of every state are the sheet's. The state that changes most is the error
    // one: margin 0 and a 2px border of #E25243 (ComboBox.qss:44-51), the "inside margins" of the Gallery's Combo Box page.
    // ----------------------------------------------------------------------------------

    namespace FieldComboBoxTestsInternal
    {
        using namespace FieldTwins;

        class FieldComboBoxTests
            : public Fixture
        {
        };

        //! A validator that rejects every item: the combo box is in error (ComboBoxWatcher::isError asks it for the current index)
        class RejectingValidator
            : public ComboBoxValidator
        {
        public:
            using ComboBoxValidator::ComboBoxValidator;

            QValidator::State validateIndex(int) const override
            {
                return QValidator::Invalid;
            }
        };

        //! The combo boxes whose sizes and rects are compared: QComboBox sizes itself with its own state, which a popup that is not open does
        //! not change
        const char* const g_names[] = { "framed", "frameless", "editable", "error", "errorFrameless", "disabled" };

        //! The same and the ones with the popup open: the sheet reads [HasPopupOpen=true] from the property that QComboBox's
        //! popup handling keeps (ComboBoxWatcher), the flattened style the state QStyle::State_On that QComboBox::initStyleOption gives
        //! while it shows the popup (qcombobox.cpp:1499-1500)
        const char* const g_paintNames[] = { "framed", "frameless", "editable", "error", "errorFrameless", "disabled", "popup", "errorPopup" };

        //! One tree of combo boxes: the sheet reads HasError and HasPopupOpen from the widgets, the flattened style as well
        void buildCombos(QWidget& root, bool legacy)
        {
            Q_UNUSED(legacy);
            auto* layout = new QVBoxLayout(&root);
            const struct
            {
                const char* m_name;
                bool m_frame;
                bool m_editable;
                bool m_error;
                bool m_enabled;
                bool m_popup;
            } combos[] = {
                { "framed", true, false, false, true, false },
                { "frameless", false, false, false, true, false },
                { "editable", true, true, false, true, false },
                { "error", true, false, true, true, false },
                { "errorFrameless", false, false, true, true, false },
                { "disabled", true, false, false, false, false },
                { "popup", true, false, false, true, true },
                { "errorPopup", true, false, true, true, true },
            };
            for (const auto& entry : combos)
            {
                auto* combo = new QComboBox(&root);
                combo->setObjectName(QString::fromLatin1(entry.m_name));
                combo->addItems({ QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large") });
                combo->setFrame(entry.m_frame);
                combo->setEditable(entry.m_editable);
                combo->setEnabled(entry.m_enabled);
                if (entry.m_error)
                {
                    // The flattened GUI keeps the state by its validator (ComboBoxWatcher::updateErrorState); the legacy sheet reads the
                    // property
                    ComboBox::setValidator(combo, new RejectingValidator(combo));
                    combo->setProperty(HasError, true);
                }
                if (entry.m_popup)
                {
                    combo->setProperty(HasPopupOpen, true);
                }
                combo->setFixedWidth(200);
                if (entry.m_popup && entry.m_error)
                {
                    // The layout sized this combo box before its popup opened (20 high); the sheet, with the property, would size it
                    // 18 high now (QComboBox does not size itself again for the state of a popup it does not show)
                    combo->setFixedHeight(20);
                }
                layout->addWidget(combo);
            }
            layout->addStretch();
        }

        //! The option QComboBox::paintEvent builds (QComboBox::initStyleOption is protected), in a state added to the widget's own
        QStyleOptionComboBox optionOf(const QComboBox* combo, QStyle::State extraState)
        {
            QStyleOptionComboBox option;
            option.initFrom(combo);
            option.state |= extraState;
            option.rect = combo->rect();
            option.editable = combo->isEditable();
            option.frame = combo->hasFrame();
            option.currentText = combo->currentText();
            option.iconSize = combo->iconSize();
            option.subControls = QStyle::SC_All;
            option.activeSubControls = QStyle::SC_None;
            return option;
        }

        //! The states a combo box that can be hovered and focused is judged in; a disabled one has neither, one with its popup open has
        //! the state of the popup (with the focus it took, or without)
        std::vector<QStyle::State> statesOf(const char* name)
        {
            if (qstrcmp(name, "disabled") == 0)
            {
                return { QStyle::State() };
            }
            if (QString::fromLatin1(name).endsWith(QStringLiteral("opup")))
            {
                return { QStyle::State(QStyle::State_On), QStyle::State(QStyle::State_On | QStyle::State_HasFocus) };
            }
            return { QStyle::State(), QStyle::State(QStyle::State_MouseOver), QStyle::State(QStyle::State_HasFocus) };
        }

        //! A combo box as QComboBox::paintEvent draws it: the box and its arrow, then the label
        QImage paintCombo(const QComboBox* combo, qreal ratio, QStyle::State extraState)
        {
            return paintThroughStyle(combo, ratio, [combo, extraState](QPainter& painter)
            {
                const QStyleOptionComboBox option = optionOf(combo, extraState);
                combo->style()->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, combo);
                combo->style()->drawControl(QStyle::CE_ComboBoxLabel, &option, &painter, combo);
            });
        }

        // ----------------------------------------------------------------------------------
        // T-CB.1: the size
        // ----------------------------------------------------------------------------------

        TEST_F(FieldComboBoxTests, Hints_AreTheSheetsWhateverTheFrame)
        {
            ASSERT_TRUE(StyleManager::stylesheetsDisabled());

            Twins twins(*this, buildCombos, QSize(300, 420));

            for (const char* name : g_names)
            {
                const auto* legacy = twins.legacyChild<QComboBox>(name);
                const auto* flat = twins.flatChild<QComboBox>(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                EXPECT_TRUE(flat->sizeHint() == legacy->sizeHint()) << name << ": " << sizeText(flat->sizeHint()) << " against " << sizeText(legacy->sizeHint());
                EXPECT_TRUE(flat->minimumSizeHint() == legacy->minimumSizeHint())
                    << name << ": " << sizeText(flat->minimumSizeHint()) << " against " << sizeText(legacy->minimumSizeHint());
                EXPECT_EQ(flat->height(), legacy->height()) << name;
            }

            // The frame changes nothing, and the error state is 2 higher and 2 wider (ComboBox.qss:46-47: 16 + 2 x 1 of margin, and
            // 16 + 2 x 2 of border)
            const auto* framed = twins.flatChild<QComboBox>("framed");
            EXPECT_TRUE(twins.flatChild<QComboBox>("frameless")->sizeHint() == framed->sizeHint());
            EXPECT_EQ(framed->sizeHint().height(), 18);
            EXPECT_EQ(twins.flatChild<QComboBox>("error")->sizeHint().height(), 20);
            EXPECT_TRUE(twins.flatChild<QComboBox>("errorFrameless")->sizeHint() == twins.flatChild<QComboBox>("error")->sizeHint());
            EXPECT_EQ(twins.flatChild<QComboBox>("error")->sizeHint().width(), framed->sizeHint().width() + 2);
        }

        // ----------------------------------------------------------------------------------
        // T-CB.2: the sub-controls
        // ----------------------------------------------------------------------------------

        TEST_F(FieldComboBoxTests, SubControls_AreTheSheetsInEveryState)
        {
            Twins twins(*this, buildCombos, QSize(300, 420));

            for (const char* name : g_paintNames)
            {
                const auto* legacy = twins.legacyChild<QComboBox>(name);
                const auto* flat = twins.flatChild<QComboBox>(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                for (const QStyle::State state : statesOf(name))
                {
                    const QStyleOptionComboBox legacyOption = optionOf(legacy, state);
                    const QStyleOptionComboBox flatOption = optionOf(flat, state);
                    for (const QStyle::SubControl subControl : { QStyle::SC_ComboBoxFrame, QStyle::SC_ComboBoxEditField, QStyle::SC_ComboBoxArrow })
                    {
                        const QRect expected = legacy->style()->subControlRect(QStyle::CC_ComboBox, &legacyOption, subControl, legacy);
                        const QRect actual = flat->style()->subControlRect(QStyle::CC_ComboBox, &flatOption, subControl, flat);
                        EXPECT_TRUE(actual == expected) << name << " state " << int(state) << " sub-control " << int(subControl) << ": "
                                                        << rectText(actual) << " against " << rectText(expected);
                    }
                }
            }
        }

        TEST_F(FieldComboBoxTests, EditField_OfAnEditableComboBoxIsTheLineEditOfTheSheet)
        {
            Twins twins(*this, buildCombos, QSize(300, 420));

            const auto* legacy = twins.legacyChild<QComboBox>("editable");
            const auto* flat = twins.flatChild<QComboBox>("editable");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            ASSERT_TRUE(legacy->lineEdit() != nullptr && flat->lineEdit() != nullptr);

            // n2 Gallery 08_combo-box: the editable combo box is 546 x 18 and its line edit 524 x 16 at (5, 1)
            EXPECT_TRUE(flat->lineEdit()->geometry() == legacy->lineEdit()->geometry())
                << rectText(flat->lineEdit()->geometry()) << " against " << rectText(legacy->lineEdit()->geometry());
            EXPECT_TRUE(flat->lineEdit()->geometry() == QRect(5, 1, flat->width() - 22, 16)) << rectText(flat->lineEdit()->geometry());
        }

        TEST_F(FieldComboBoxTests, ErrorButton_KeepsClearOfTheDropDownOfTheSheet)
        {
            Twins twins(*this, buildCombos, QSize(300, 420));

            // The watcher places the button from the width of the drop-down (ComboBoxWatcher::positionSideWidgets, asked with a null
            // rect and no state): the sheet's is 16, Fusion's 19
            const auto* legacy = twins.legacyChild<QComboBox>("error");
            auto* flat = twins.flatChild<QComboBox>("error");
            ASSERT_TRUE(legacy != nullptr && flat != nullptr);
            QStyleOptionComboBox nullOption;
            nullOption.subControls &= QStyle::SC_ComboBoxArrow;
            const QRect expected = legacy->style()->subControlRect(QStyle::CC_ComboBox, &nullOption, QStyle::SC_ComboBoxArrow, legacy);
            const QRect actual = flat->style()->subControlRect(QStyle::CC_ComboBox, &nullOption, QStyle::SC_ComboBoxArrow, flat);
            EXPECT_EQ(actual.width(), expected.width());
            EXPECT_EQ(actual.width(), 16);

            // n2 Gallery 08_combo-box: the error combo box is 546 wide and its ErrorToolButton at x 509 = 546 - 37 (the border 1, the
            // button 16, the drop-down 16 and the spacing 4)
            auto* button = flat->findChild<QToolButton*>(ErrorToolButton);
            ASSERT_TRUE(button != nullptr);
            QApplication::processEvents();
            EXPECT_EQ(button->x(), flat->width() - 37);
            EXPECT_EQ(button->y(), (flat->height() - button->height()) / 2);
        }

        // ----------------------------------------------------------------------------------
        // T-CB.3: the pixels, in every state and at the ratios of the captures
        // ----------------------------------------------------------------------------------

        TEST_F(FieldComboBoxTests, Paint_IsTheSheetsInEveryStateAndRatio)
        {
            Twins twins(*this, buildCombos, QSize(300, 420));

            for (const char* name : g_paintNames)
            {
                const auto* legacy = twins.legacyChild<QComboBox>(name);
                const auto* flat = twins.flatChild<QComboBox>(name);
                ASSERT_TRUE(legacy != nullptr && flat != nullptr) << name;
                for (const qreal ratio : { 1.0, 1.5, 2.0 })
                {
                    for (const QStyle::State state : statesOf(name))
                    {
                        const QImage expected = paintCombo(legacy, ratio, state);
                        const QImage actual = paintCombo(flat, ratio, state);
                        EXPECT_TRUE(isDrawnOn(expected)) << name << " ratio " << ratio << " state " << int(state);
                        EXPECT_EQ(differing(actual, expected), 0)
                            << name << " ratio " << ratio << " state " << int(state) << ": " << firstDifference(actual, expected);
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // T-CB.4: the error state follows the property (the watcher resolves the element of the combo box again)
        // ----------------------------------------------------------------------------------

        TEST_F(FieldComboBoxTests, Error_IsTheBoxOfThePropertyWhicheverWayItIsSetAndCleared)
        {
            QWidget root;
            auto* combo = new QComboBox(&root);
            combo->addItems({ QStringLiteral("Small"), QStringLiteral("Medium") });
            combo->setFixedWidth(200);
            flat(root);
            show(root, QSize(210, 40));

            const QSize idle = combo->sizeHint();
            EXPECT_EQ(idle.height(), 18);

            // ComboBox.qss:44-51 [HasError=true]: margin 0 and a 2px border round the 16px contents
            combo->setProperty(HasError, true);
            EXPECT_EQ(combo->sizeHint().height(), 20);
            EXPECT_EQ(combo->sizeHint().width(), idle.width() + 2);

            // and the idle box is back with the property
            combo->setProperty(HasError, false);
            EXPECT_TRUE(combo->sizeHint() == idle) << sizeText(combo->sizeHint());
        }

        // ----------------------------------------------------------------------------------
        // T-CB.5: a property that comes and goes with the popup (HasPopupOpen, twice for each time the popup is used) does not polish the
        // combo box again: a polish connects the signals of the combo box to its watcher once more, and each of them is one more call on
        // every change of its index for as long as the combo box lives
        // ----------------------------------------------------------------------------------

        //! A combo box that tells how many slots its index changes call (QObject::receivers is protected)
        class CountingComboBox
            : public QComboBox
        {
        public:
            using QComboBox::QComboBox;

            int indexChangedReceivers() const
            {
                return receivers(SIGNAL(currentIndexChanged(int)));
            }
        };

        TEST_F(FieldComboBoxTests, Popup_OpeningAndClosingItDoesNotConnectTheComboBoxAgain)
        {
            QWidget root;
            auto* combo = new CountingComboBox(&root);
            combo->addItems({ QStringLiteral("Small"), QStringLiteral("Medium") });
            combo->setFixedWidth(200);
            flat(root);
            show(root, QSize(210, 40));

            const int connected = combo->indexChangedReceivers();
            EXPECT_GT(connected, 0);        // the watcher of the error state
            for (int use = 0; use < 4; ++use)
            {
                combo->setProperty(HasPopupOpen, true);
                combo->setProperty(HasPopupOpen, false);
            }
            EXPECT_EQ(combo->indexChangedReceivers(), connected);

            // and the box it draws is the one it had
            const QSize idle = combo->sizeHint();
            EXPECT_EQ(idle.height(), 18);
        }
    } // namespace FieldComboBoxTestsInternal
} // namespace AzQtComponents

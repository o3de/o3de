/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Tests/SheetTwins.h>

#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>

#include <QEnterEvent>
#include <QLineEdit>
#include <QStyleOptionSpinBox>

namespace AzQtComponents
{
    // ----------------------------------------------------------------------------------
    // The error face of a spin box (SpinBox.qss:57-67) and the places where it is laid out:
    //   - A13: the rule names two states, `[hasAcceptableInput=false]` and `[HasError=true]`. QAbstractSpinBox has no property of the
    //     first name (its own is `acceptableInput`), so the first state is a dynamic property a caller would have to set, and a box
    //     with unacceptable input is drawn as any other;
    //   - A14: a HasError that changes while the box is shown. The error face of a spin box in a vector element puts back the 2px
    //     at the left that the vector element took away (VectorInput.qss:18-28 against SpinBox.qss:57-67), so the line edit of the
    //     box has a new place. The sheet style lays it out when QAbstractSpinBox does, and so does the flattened style.
    // The legacy twin is Qt's own style sheet style over AzQtComponents::Style with the application's real sheet; the flat twin is the
    // flattened Style (SheetTwins.h).
    // ----------------------------------------------------------------------------------

    namespace SpinBoxErrorFlatSheetTestsInternal
    {
        using namespace SheetTwins;

        class SpinBoxErrorFlatSheetTests
            : public ::UnitTest::LeakDetectionFixture
        {
        protected:
            Context m_context;
        };

        // ----------------------------------------------------------------------------------
        // Rows and states
        // ----------------------------------------------------------------------------------

        //! A spin box as the GUI builds it (the library's own class), at the size an inspector row gives it, in the place the row names
        Row spinBoxRow(const char* site, const std::vector<State>& states)
        {
            return Row{
                .m_site = site,
                .m_make = [](QWidget* parent)
                {
                    auto* spinBox = new DoubleSpinBox(parent);
                    spinBox->setObjectName(QStringLiteral("spin"));
                    spinBox->setRange(0.0, 100.0);
                    spinBox->setValue(50.0);
                    return static_cast<QWidget*>(spinBox);
                },
                .m_realApplicationSheet = true,
                .m_states = states,
                // The palette of the box itself is left out: the sheet gave the box the colours of the field, where the flattened
                // style draws the field in the element and keeps the text colours in the palette of the line edit (SpinBox::polish)
                .m_compare = Compare::Geometry | Compare::Fonts | Compare::Pixels,
                .m_size = QSize(120, 23),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        QLineEdit* lineEditOf(QWidget& spinBox)
        {
            return spinBox.findChild<QLineEdit*>();
        }

        //! Text in the field that its validator does not accept (the box is a number): QAbstractSpinBox::hasAcceptableInput is false
        State unacceptableInput()
        {
            return { "unacceptable input",
                [](QWidget& widget)
                {
                    lineEditOf(widget)->setText(QStringLiteral("abc"));
                    EXPECT_FALSE(qobject_cast<QAbstractSpinBox*>(&widget)->hasAcceptableInput());
                },
                [](QWidget& widget)
                {
                    auto* spinBox = qobject_cast<QDoubleSpinBox*>(&widget);
                    lineEditOf(widget)->setText(spinBox->textFromValue(spinBox->value()));
                    EXPECT_TRUE(spinBox->hasAcceptableInput());
                } };
        }

        //! The state the sheet names, `[hasAcceptableInput=false]`: a dynamic property, as no property of the class has the name
        State propertyFalse()
        {
            return States::property("hasAcceptableInput", false);
        }

        // ----------------------------------------------------------------------------------
        // A13
        // ----------------------------------------------------------------------------------

        TEST_F(SpinBoxErrorFlatSheetTests, A13_TheClassHasNoPropertyOfTheNameTheSheetSelects)
        {
            RungScope rung(Rung::Flat);
            DoubleSpinBox spinBox;
            const QMetaObject* meta = spinBox.metaObject();

            // The selector `[hasAcceptableInput=false]` reads a property of that name (QStyleSheetStyleSelector::attributeValue); the
            // class has `acceptableInput`, and no widget in the tree sets one of the other name
            EXPECT_GE(meta->indexOfProperty("acceptableInput"), 0);
            EXPECT_EQ(meta->indexOfProperty("hasAcceptableInput"), -1);
            EXPECT_FALSE(spinBox.property("hasAcceptableInput").isValid());
        }

        TEST_F(SpinBoxErrorFlatSheetTests, A13_LegacyDrawsNothingForUnacceptableInputAndTheErrorFaceForThePropertyOnly)
        {
            Row row = spinBoxRow("A13 a spin box", { unacceptableInput(), propertyFalse() });
            row.m_compare = Compare::Pixels;

            RungScope rung(Rung::Qss);
            const Twin twin(row, m_context, true);
            const std::vector<Reading> readings = readStates(row, *twin.widget(), m_context, true, nullptr);
            ASSERT_EQ(readings.size(), 3u);

            for (size_t picture = 0; picture < readings[0].m_pictures.size(); ++picture)
            {
                const QImage& idle = readings[0].m_pictures[picture].m_image;
                ASSERT_TRUE(FieldTwins::isDrawnOn(idle));

                // The two rows above the field: the margin of the box at rest (SpinBox.qss:13), where the error face has its border
                // (SpinBox.qss:62-66, margin 0 and 2px of red). The text of the field is not in them.
                const int rows = qRound(2 * idle.devicePixelRatio());
                const auto margin = [rows](const QImage& image) { return image.copy(QRect(0, 0, image.width(), rows)); };

                EXPECT_EQ(FieldTwins::differing(margin(readings[1].m_pictures[picture].m_image), margin(idle)), 0)
                    << readings[0].m_pictures[picture].m_name;
                EXPECT_NE(FieldTwins::differing(margin(readings[2].m_pictures[picture].m_image), margin(idle)), 0)
                    << readings[0].m_pictures[picture].m_name;
            }
        }

        TEST_F(SpinBoxErrorFlatSheetTests, A13_UnacceptableInputIsDrawnAsAnyOtherInputInBothGuis)
        {
            EXPECT_TRUE(check(spinBoxRow("A13 a spin box with unacceptable input", { unacceptableInput() }), m_context));
        }

        TEST_F(SpinBoxErrorFlatSheetTests, A13_ThePropertyTheSheetNamesGivesTheErrorFaceInBothGuis)
        {
            EXPECT_TRUE(check(spinBoxRow("A13 a spin box with [hasAcceptableInput=false]", { propertyFalse() }), m_context));
        }

        TEST_F(SpinBoxErrorFlatSheetTests, A13_ADisabledBoxHasNoErrorFaceInEitherGui)
        {
            // `:enabled[hasAcceptableInput=false]`: the face of a disabled box is the disabled one, whatever the property says
            const State disabledWithProperty{ "disabled, [hasAcceptableInput=false]",
                [](QWidget& widget)
                {
                    widget.setProperty("hasAcceptableInput", false);
                    widget.setEnabled(false);
                    States::repolish(widget);
                },
                [](QWidget& widget)
                {
                    widget.setProperty("hasAcceptableInput", QVariant());
                    widget.setEnabled(true);
                    States::repolish(widget);
                } };
            EXPECT_TRUE(check(spinBoxRow("A13 a disabled spin box with [hasAcceptableInput=false]", { disabledWithProperty }), m_context));
        }

        TEST_F(SpinBoxErrorFlatSheetTests, A13_OnlyTheValueFalseGivesTheErrorFaceInBothGuis)
        {
            // The selector compares the text of the property (QCss::StyleSelector::basicSelectorMatches): a bool true, the text "false"
            // and a number are what they are in the sheet. 0 is not the text "false".
            const State trueValue = States::property("hasAcceptableInput", true);
            const State zero = States::property("hasAcceptableInput", 0);
            const State text = States::property("hasAcceptableInput", QStringLiteral("false"));
            EXPECT_TRUE(check(spinBoxRow("A13 [hasAcceptableInput] true, 0 and the text false", { trueValue, zero, text }), m_context));
        }

        // ----------------------------------------------------------------------------------
        // A14
        // ----------------------------------------------------------------------------------

        //! Where the line edit of a spin box is and where the style says the edit field is
        struct FieldReading
        {
            QRect m_lineEdit;
            QRect m_field;
        };

        FieldReading readField(QAbstractSpinBox* spinBox)
        {
            QStyleOptionSpinBox option;
            option.initFrom(spinBox);
            option.rect = spinBox->rect();
            option.buttonSymbols = spinBox->buttonSymbols();
            option.subControls = QStyle::SC_All;
            option.frame = true;
            return { spinBox->findChild<QLineEdit*>()->geometry(),
                spinBox->style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, spinBox) };
        }

        //! One step of the drive of a spin box in a vector element: what is done, and what the box looked like after it
        struct Step
        {
            std::string m_name;
            FieldReading m_reading;
        };

        //! A spin box in a vector element, 160 by 23, as the transform rows of the inspector have them
        Row vectorRow()
        {
            return Row{
                .m_site = "A14 the spin box of a vector element",
                .m_make = [](QWidget* parent)
                {
                    auto* element = new VectorElement(parent);
                    element->setLabel(QStringLiteral("X"));
                    element->setCoordinate(VectorElement::Coordinate::X);
                    return static_cast<QWidget*>(element);
                },
                .m_realApplicationSheet = true,
                .m_compare = Compare::Geometry,
                .m_size = QSize(160, 23),
                .m_base = Base::AzQtComponentsStyle,
            };
        }

        //! The drive: the error comes and goes while the box is shown, the pointer enters and leaves it, the box is resized. The change of
        //! the property is followed by the unpolish and polish the watcher of the spin box does through the style
        //! (SpinBoxWatcher, QEvent::DynamicPropertyChange): StyleManager::styleSheetStyle is that style in either GUI.
        std::vector<Step> drive(QWidget* element)
        {
            auto* spinBox = element->findChild<QAbstractSpinBox*>();
            std::vector<Step> steps;
            const auto note = [&](const char* name) { steps.push_back({ name, readField(spinBox) }); };
            const auto setError = [&](bool error)
            {
                SpinBox::setHasError(spinBox, error);
                spinBox->style()->unpolish(spinBox);
                spinBox->style()->polish(spinBox);
                QApplication::processEvents();
            };
            const auto pointer = [&](bool inside)
            {
                if (inside)
                {
                    QEnterEvent enter(QPointF(20, 10), QPointF(20, 10), QPointF(20, 10));
                    QApplication::sendEvent(spinBox, &enter);
                }
                else
                {
                    QEvent leave(QEvent::Leave);
                    QApplication::sendEvent(spinBox, &leave);
                }
                QApplication::processEvents();
            };

            note("shown");
            setError(true);
            note("error set");
            pointer(true);
            note("error set, pointer entered");
            pointer(false);
            note("error set, pointer left");
            setError(false);
            note("error cleared");
            pointer(true);
            note("error cleared, pointer entered");
            pointer(false);
            note("error cleared, pointer left");
            setError(true);
            spinBox->resize(spinBox->width() + 1, spinBox->height());
            QApplication::processEvents();
            note("error set, resized");
            return steps;
        }

        std::vector<Step> driveTwin(const Row& row, bool legacy, Context& context)
        {
            RungScope rung(legacy ? Rung::Qss : Rung::Flat);
            const Twin twin(row, context, legacy);
            return drive(twin.widget());
        }

        TEST_F(SpinBoxErrorFlatSheetTests, A14_TheLineEditOfAVectorElementIsLaidOutWhenTheLegacyBoxIsAndWhenTheFlatBoxIs)
        {
            const Row row = vectorRow();
            const std::vector<Step> legacy = driveTwin(row, true, m_context);
            const std::vector<Step> flat = driveTwin(row, false, m_context);
            ASSERT_EQ(legacy.size(), flat.size());

            for (size_t i = 0; i < legacy.size(); ++i)
            {
                EXPECT_EQ(FieldTwins::rectText(flat[i].m_reading.m_lineEdit), FieldTwins::rectText(legacy[i].m_reading.m_lineEdit))
                    << "line edit, " << legacy[i].m_name;
                EXPECT_EQ(FieldTwins::rectText(flat[i].m_reading.m_field), FieldTwins::rectText(legacy[i].m_reading.m_field))
                    << "edit field, " << legacy[i].m_name;
            }
        }

        TEST_F(SpinBoxErrorFlatSheetTests, A14_TheLegacyLineEditStaysWhereItWasUntilThePointerOrAResizeLaysItOutAgain)
        {
            const Row row = vectorRow();
            const std::vector<Step> legacy = driveTwin(row, true, m_context);
            ASSERT_EQ(legacy.size(), 8u);

            // The edit field of the style follows the property at once; the line edit is where the last layout put it
            EXPECT_EQ(legacy[0].m_reading.m_field.left(), 0) << legacy[0].m_name;
            EXPECT_EQ(legacy[1].m_reading.m_field.left(), 2) << legacy[1].m_name;
            EXPECT_EQ(legacy[1].m_reading.m_lineEdit.left(), legacy[0].m_reading.m_lineEdit.left()) << "no layout: " << legacy[1].m_name;
            EXPECT_EQ(legacy[2].m_reading.m_lineEdit.left(), 2) << legacy[2].m_name;
            EXPECT_EQ(legacy[3].m_reading.m_lineEdit.left(), 2) << legacy[3].m_name;
            EXPECT_EQ(legacy[4].m_reading.m_field.left(), 0) << legacy[4].m_name;
            EXPECT_EQ(legacy[4].m_reading.m_lineEdit.left(), 2) << "no layout: " << legacy[4].m_name;
            EXPECT_EQ(legacy[5].m_reading.m_lineEdit.left(), 0) << legacy[5].m_name;
            EXPECT_EQ(legacy[7].m_reading.m_lineEdit.left(), 2) << legacy[7].m_name;
        }
    } // namespace SpinBoxErrorFlatSheetTestsInternal
} // namespace AzQtComponents

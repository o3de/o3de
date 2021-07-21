/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SpinBoxPage.h"
#include <AzQtComponents/Gallery/ui_SpinBoxPage.h>

#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QUndoCommand>
#include <QPointer>
#include <QEvent>
#include <QAction>
#include <QMenu>
#include <QDebug>

template<typename Spinbox, typename ValueType>
class SpinBoxChangedCommand : public QUndoCommand
{
public:
    SpinBoxChangedCommand(Spinbox* objectChanging, ValueType oldValue, ValueType newValue)
        : QUndoCommand(QStringLiteral("Change %1 to %2").arg(oldValue).arg(newValue))
        , m_spinBox(objectChanging)
        , m_oldValue(oldValue)
        , m_newValue(newValue)
    {
    }

    void undo() override
    {
        m_spinBox->setValue(m_oldValue);

        if (m_spinBox->hasFocus())
        {
            m_spinBox->selectAll();
        }
    }

    void redo() override
    {
        if (!m_firstTime)
        {
            m_spinBox->setValue(m_newValue);

            if (m_spinBox->hasFocus())
            {
                m_spinBox->selectAll();
            }
        }

        m_firstTime = false;
    }

private:
    QPointer<Spinbox> m_spinBox;

    bool m_firstTime = true;
    ValueType m_oldValue;
    ValueType m_newValue;
};

SpinBoxPage::SpinBoxPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::SpinBoxPage)
{
    ui->setupUi(this);
    ui->focusSpinBox->setFocus();
    ui->filledInDoubleSpinBox->setDecimals(40);
    ui->filledInDoubleSpinBox->setDisplayDecimals(2);
    ui->filledInDoubleSpinBox->setValue(3.1415926535897932384626433832795028841971);
    setFocusProxy(ui->focusSpinBox);

    ui->focusDoubleSpinBox->setRange(0.0, 1.0);
    ui->focusDoubleSpinBox->setSingleStep(0.01);
    ui->focusDoubleSpinBox->setValue(0.07);

    AzQtComponents::SpinBox::setHasError(ui->hasErrorSpinBox, true);
    AzQtComponents::SpinBox::setHasError(ui->hasErrorDoubleSpinBox, true);

    ui->metersVectorInput->setSuffix(" m");

    {
        const QStringList labels = {"Top", "Right", "Bottom", "Left"};
        const int size = ui->labelsVectorInput->getSize();
        for (int i = 0; i < size; ++i)
        {
            ui->labelsVectorInput->setLabel(i, labels[i % labels.size()]);
        }
    }

    {
        const QStringList styles =
        {
            "font: bold; color: rgb(184,51,51);",
            "font: bold; color: rgb(48,208,120);",
            "font: bold; color: rgb(66,133,244);"
        };
        const int size = ui->styleVectorInput->getSize();
        for (int i = 0; i < size; ++i)
        {
            ui->styleVectorInput->setLabelStyle(i, styles[i % styles.size()]);
        }
    }

    ui->horizontalLayout->setStretch(0, 3);
    ui->horizontalLayout->setStretch(1, 1);

    QString exampleText = R"(

QAbstractSpinBox docs: <a href="https://doc.qt.io/qt-5/qabstractspinbox.html">https://doc.qt.io/qt-5/qabstractspinbox.html</a><br/>
QSpinBox docs: <a href="https://doc.qt.io/qt-5/qspinbox">https://doc.qt.io/qt-5/qspinbox</a><br/>
QDoubleSpinBox docs: <a href="https://doc.qt.io/qt-5/qdoublespinbox">https://doc.qt.io/qt-5/qdoublespinbox</a><br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/SpinBox.h&gt;

AzQtComponents::DoubleSpinBox* doubleSpinBox;

// Assuming you've created a DoubleSpinBox already (either in code or via .ui file):

// To set the range to 0 - 1 and step by 0.01:
doubleSpinBox->setRange(0.0, 1.0);
doubleSpinBox->setSingleStep(0.01);

// By default, QAbstractSpinBox::hasAcceptableInput is used to display the error state.
// The developer can set the QAbstractSpinBox::CorrectionMode to control this behavior.
// In addition, the developer can use AzQtComponents::SpinBox::setHasError to manually
// override the error state of a SpinBox or DoubleSpinBox.
AzQtComponents::SpinBox::setHasError(doubleSpinBox, true);
)";

    ui->exampleText->setHtml(exampleText);

    track<AzQtComponents::SpinBox, int>(ui->defaultSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->defaultDoubleSpinBox);
    track<AzQtComponents::SpinBox, int>(ui->errorSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->errorDoubleSpinBox);
    track<AzQtComponents::SpinBox, int>(ui->filledInSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->filledInDoubleSpinBox);
    track<AzQtComponents::SpinBox, int>(ui->focusSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->focusDoubleSpinBox);

    {
        QAction* action = new QAction("Up", this);
        action->setShortcut(QKeySequence(Qt::Key_Up));
        connect(action, &QAction::triggered, [this]()
        {
            qDebug() << "Up pressed";
        });
        addAction(action);
    }
    {
        QAction* action = new QAction("Down", this);
        action->setShortcut(QKeySequence(Qt::Key_Down));
        connect(action, &QAction::triggered, [this]()
        {
            qDebug() << "Down pressed";
        });
        addAction(action);
    }
}

SpinBoxPage::~SpinBoxPage()
{
}

template <typename SpinBoxType, typename ValueType>
void SpinBoxPage::track(SpinBoxType* spinBox)
{
    // connect to changes in the spinboxes and listen for the undo state
    QObject::connect(spinBox, &SpinBoxType::valueChangeBegan, this, [this, spinBox]() {
        ValueType oldValue = spinBox->value();
        spinBox->setProperty("OldValue", oldValue);
    });

    QObject::connect(spinBox, &SpinBoxType::valueChangeEnded, this, [this, spinBox]() {
        ValueType oldValue = spinBox->property("OldValue").template value<ValueType>();
        ValueType newValue = spinBox->value();

        if (oldValue != newValue)
        {
            m_undoStack.push(new SpinBoxChangedCommand<SpinBoxType, ValueType>(spinBox, oldValue, newValue));

            spinBox->setProperty("OldValue", newValue);
        }
    });

    QObject::connect(spinBox, &SpinBoxType::globalUndoTriggered, &m_undoStack, &QUndoStack::undo);
    QObject::connect(spinBox, &SpinBoxType::globalRedoTriggered, &m_undoStack, &QUndoStack::redo);

    QObject::connect(spinBox, &SpinBoxType::contextMenuAboutToShow, this, [this](QMenu* menu, QAction* undo, QAction* redo) {
        Q_UNUSED(menu);

        undo->setEnabled(m_undoStack.canUndo());
        redo->setEnabled(m_undoStack.canRedo());
    });
}

#include "Gallery/moc_SpinBoxPage.cpp"


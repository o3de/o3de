/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzCore/std/optional.h>
#endif

class QLabel;

namespace AzQtComponents
{

    class Style;

    // 'AzQtComponents::VectorElement::m_deferredExternalValue': class 'AZStd::optional<AzQtComponents::VectorElement::DeferredSetValue>' needs to
    // have dll-interface to be used by clients of class 'AzQtComponents::VectorElement' 
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    
    /*!
     * \class VectorElement
     * \brief All flexible vector GUI's are constructed using a number vector elements. Each Vector
     * element contains a label and a double spin box which show the label and the value respectively.
     */
    class AZ_QT_COMPONENTS_API VectorElement
        : public QWidget
    {
        Q_OBJECT
    public:
        enum class Coordinate
        {
            X,
            Y,
            Z,
            W
        };
        Q_ENUM(Coordinate)

        explicit VectorElement(QWidget* parent = nullptr);

        /**
        * Sets the label for this vector element
        * @param label The new label
        */
        void SetLabel(const char* label);
        void setLabel(const QString& label);

        const QString& label() const;

        QLabel* GetLabel() const { return getLabelWidget(); }
        QLabel* getLabelWidget() const { return m_label; }

        /**
        * Sets the value for the spinbox in this vector element
        * @param newValue The new value
        */
        void SetValue(double newValue) { setValue(newValue); }
        void setValue(double newValue);

        double GetValue() const { return getValue(); }
        double getValue() const { return m_value; }

        void setCoordinate(Coordinate coord);

        AzQtComponents::DoubleSpinBox* GetSpinBox() const { return getSpinBox(); }
        AzQtComponents::DoubleSpinBox* getSpinBox() const
        {
            return m_spinBox;
        }

        bool WasValueEditedByUser() const { return wasValueEditedByUser(); }
        inline bool wasValueEditedByUser() const
        {
            return m_wasValueEditedByUser;
        }

        QSize sizeHint() const override;

    Q_SIGNALS:
        void valueChanged(double);
        void editingFinished();

    public Q_SLOTS:
        void onValueChanged(double newValue);

    protected:
        friend class Style;

        static void layout(QWidget* element, QAbstractSpinBox* spinBox, QLabel* label, bool ui20);

        static QRect editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const SpinBox::Config& config);

        static bool polish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config);
        static bool unpolish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config);

        static void initStaticVars(int labelSize);

        void resizeLabel();

        void onSpinBoxEditingFinished();

        virtual void changeEvent(QEvent* event) override;
    private:
        struct DeferredSetValue
        {
            double prevValue;
            double value;
        };

        // m_labelText must be initialised before m_spinBox. It is used by editFieldRect, which gets
        // called by the spin box constructor.
        QString m_labelText = {};
        DoubleSpinBox* m_spinBox = nullptr;
        QLabel* m_label = nullptr;

        double m_value = 0.0;
        //! Indicates whether the value in the spin box has been edited by the user or not
        bool m_wasValueEditedByUser = false;
        //! If a value is editing, but not by the user, and the user is currently editing the value,
        //! avoid overwriting their work, until they finish editing
        AZStd::optional<DeferredSetValue> m_deferredExternalValue;
    };

    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    //////////////////////////////////////////////////////////////////////////

    /*!
     * \class VectorInput
     * \brief Qt Widget that control holds an array of VectorElements.
     * This control can be used to display any number of labeled float values with configurable row(s) & column(s)
     */
    class AZ_QT_COMPONENTS_API VectorInput
        : public QWidget
    {
        Q_OBJECT
    public:

        /**
        * Configures and constructs a vector control
        * @param parent The Parent Qwidget
        * @param elementCount Number of elements being managed by this vector control
        * @param elementsPerRow Number of elements in every row
        * @param customLabels A string that has custom labels that are use by the Vector elements
        */
        explicit VectorInput(QWidget* parent = nullptr, int elementCount = 4, int elementsPerRow = -1, QString customLabels = "");
        ~VectorInput() override;

        /**
        * Sets the label on the indicated Vector element
        * @param index Index of the element for which the label is to be set
        * @param label New label
        */
        void setLabel(int index, const QString& label);

        /**
        * Sets the style on the indicated Vector element
        * @param index Index of the element for which the label is to be styled
        * @param qss The Qt Style to be applied
        */
        void setLabelStyle(int index, const QString& qss);

        /**
        * Fetches all elements being managed by this Vector control
        * @return An array of VectorElement*
        */
        VectorElement** getElements()
        {
            return m_elements;
        }

        /**
        * Fetches the count of elements being managed by this control
        * @return the number of elements being managed by this control
        */
        int getSize() const
        {
            return m_elementCount;
        }

        /**
        * Sets the value on the indicated Vector element
        * @param value the new value
        * @param index Index of the element for which the value is to be set
        */
        void setValuebyIndex(double value, int index);

        /**
        * Sets the minimum value that all spinboxes being managed by this control can have
        * @param value Minimum value
        */
        void setMinimum(double value);

        /**
        * Sets the maximum value that all spinboxes being managed by this control can have
        * @param value Maximum value
        */
        void setMaximum(double value);

        /**
        * Sets the step value that all spinboxes being managed by this control can have
        * @param value Step value
        */
        void setStep(double value);

        /**
        * Sets the number of decimals to to lock the spinboxes being managed by this control to
        * @param value Decimals value
        */
        void setDecimals(int value);

        /**
        * Sets the number of display decimals to truncate the spinboxes being managed by this control to
        * @param value DisplayDecimals value
        */
        void setDisplayDecimals(int value);

        void OnValueChangedInElement(double newValue, int elementIndex);

        /**
        * Sets the suffix that is appended to values in spin boxes
        * @param suffix Suffix to be used
        */
        void setSuffix(const QString& suffix);

    Q_SIGNALS:
        void valueChanged(double);
        void valueAtIndexChanged(int elementIndex, double newValue);
        void editingFinished();

    public Q_SLOTS:
        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    private:
        //! Max size of the element vector
        int m_elementCount = 4;

        //! Array that holds any number of Vector elements, each of which represents one float value and a label in the UI
        VectorElement** m_elements = nullptr;
    };

}

Q_DECLARE_METATYPE(AzQtComponents::VectorElement::Coordinate)

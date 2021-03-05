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
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QLineEdit>
#endif

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API StyledLineEdit
        : public QLineEdit
    {
        Q_OBJECT
        Q_PROPERTY(Flavor flavor READ flavor WRITE setFlavor NOTIFY flavorChanged)

    public:

        enum Flavor
        {
            Plain = 0,
            Information,
            Question,
            Invalid,
            Valid,

            FlavorCount
        };
        Q_ENUM(Flavor)

        explicit StyledLineEdit(QWidget* parent = nullptr);
        ~StyledLineEdit();
        Flavor flavor() const;
        void setFlavor(Flavor);

    protected:
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

    signals:
        void flavorChanged();
        void onFocus(); // Required for focus dependent custom widgets, e.g. ConfigStringLineEditCtrl.
        void onFocusOut();

    private:
        void validateEntry();

        Flavor m_flavor;
    };
} // namespace AzQtComponents


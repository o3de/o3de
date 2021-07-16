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


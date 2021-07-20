#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QLineEdit>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#endif

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class ManifestNameWidget : public QLineEdit
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                explicit ManifestNameWidget(QWidget* parent);

                void SetName(const char* name);
                void SetName(const AZStd::string& name);
                const AZStd::string& GetName() const;

                void SetFilterType(const Uuid& type);
            
            signals:
                void valueChanged(const AZStd::string& newValue);

            protected:
                void OnTextChanged(const QString& textValue);

                void UpdateStatus(const AZStd::string& newName, bool checkAvailability);
                bool IsAvailableName(AZStd::string& error, const AZStd::string& name) const;

                QString m_originalToolTip;
                Uuid m_filterType;
                AZStd::string m_name;
                bool m_inFailureState;
            };
        } // SceneUI
    } // SceneAPI
} // AZ

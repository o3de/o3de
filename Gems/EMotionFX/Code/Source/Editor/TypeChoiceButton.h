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
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <QPushButton>
#endif


namespace EMotionFX
{
    class TypeChoiceButton
        : public QPushButton
    {
        Q_OBJECT //AUTOMOC

    public:
        TypeChoiceButton(const QString& text, const char* typePrefix, QWidget* parent, const AZStd::unordered_map<AZ::TypeId, AZStd::string>& types = {});
        ~TypeChoiceButton() = default;

        void SetTypes(const AZStd::unordered_map<AZ::TypeId, AZStd::string>& types);

    signals:
        void ObjectTypeChosen(AZ::TypeId type);

    public slots:
        void OnCreateContextMenu();

    private slots:
        void OnActionTriggered(bool checked);

    protected:
        AZStd::string GetNameByType(AZ::TypeId type) const;

        AZStd::unordered_map<AZ::TypeId, AZStd::string> m_types;
        AZStd::string m_typePrefix;
    };
} // namespace EMotionFX

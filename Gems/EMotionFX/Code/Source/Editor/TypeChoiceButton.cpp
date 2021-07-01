/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/TypeChoiceButton.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QMenu>


namespace EMotionFX
{
    TypeChoiceButton::TypeChoiceButton(const QString& text, const char* typePrefix, QWidget* parent, const AZStd::unordered_map<AZ::TypeId, AZStd::string>& types)
        : QPushButton(text, parent)
        , m_typePrefix(typePrefix)
        , m_types(types)
    {
        setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/ArrowDownGray.png"));
        connect(this, &QPushButton::clicked, this, &TypeChoiceButton::OnCreateContextMenu);
    }

    void TypeChoiceButton::SetTypes(const AZStd::unordered_map<AZ::TypeId, AZStd::string>& types)
    {
        m_types = types;
    }

    void TypeChoiceButton::OnCreateContextMenu()
    {
        QMenu* contextMenu = new QMenu(this);

        AZStd::string actionName;
        for (const AZStd::pair<AZ::TypeId, AZStd::string>& typePair : m_types)
        {
            const AZ::TypeId& type = typePair.first;

            if (m_typePrefix.empty())
            {
                actionName = GetNameByType(type);
            }
            else
            {
                actionName = AZStd::string::format("%s %s", m_typePrefix.c_str(), GetNameByType(type).c_str());
            }

            QAction* addBoxAction = contextMenu->addAction(actionName.c_str());
            addBoxAction->setProperty("type", type.ToString<AZStd::string>().c_str());
            connect(addBoxAction, &QAction::triggered, this, &TypeChoiceButton::OnActionTriggered);
        }

        contextMenu->setFixedWidth(width());
        connect(contextMenu, &QMenu::triggered, contextMenu, &QMenu::deleteLater);
        contextMenu->popup(mapToGlobal(QPoint(0, height())));
    }

    void TypeChoiceButton::OnActionTriggered([[maybe_unused]] bool checked)
    {
        QAction* action = static_cast<QAction*>(sender());
        const QByteArray typeString = action->property("type").toString().toUtf8();
        const AZ::TypeId& typeId = AZ::TypeId::CreateString(typeString.data(), typeString.size());

        emit ObjectTypeChosen(typeId);
    }

    AZStd::string TypeChoiceButton::GetNameByType(AZ::TypeId type) const
    {
        const auto iterator = m_types.find(type);
        if (iterator != m_types.end())
        {
            return iterator->second;
        }

        return type.ToString<AZStd::string>();
    }
} // namespace EMotionFX

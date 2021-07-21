/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "PropertyAnimationCtrl.h"

// Qt
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

// Editor
#include "Util/UIEnumerations.h"
#include "IResourceSelectorHost.h"

AnimationPropertyCtrl::AnimationPropertyCtrl(QWidget *pParent)
    : QWidget(pParent)
{
    m_animationLabel = new QLabel;

    m_pApplyButton = new QToolButton;
    m_pApplyButton->setIcon(QIcon(":/reflectedPropertyCtrl/img/apply.png"));

    m_pApplyButton->setFocusPolicy(Qt::StrongFocus);

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(m_animationLabel, 1);
    pLayout->addWidget(m_pApplyButton);

    connect(m_pApplyButton, &QAbstractButton::clicked, this, &AnimationPropertyCtrl::OnApplyClicked);
};

AnimationPropertyCtrl::~AnimationPropertyCtrl()
{
}


void AnimationPropertyCtrl::SetValue(const CReflectedVarAnimation &animation)
{
    m_animation = animation;
    m_animationLabel->setText(animation.m_animation.c_str());
}

CReflectedVarAnimation AnimationPropertyCtrl::value() const
{
    return m_animation;
}

void AnimationPropertyCtrl::OnApplyClicked()
{
    QStringList cSelectedAnimations;
    size_t nTotalAnimations(0);
    size_t nCurrentAnimation(0);

    QString combinedString = GetIEditor()->GetResourceSelectorHost()->GetGlobalSelection("animation");
    SplitString(combinedString, cSelectedAnimations, ',');

    nTotalAnimations = cSelectedAnimations.size();
    for (nCurrentAnimation = 0; nCurrentAnimation < nTotalAnimations; ++nCurrentAnimation)
    {
        QString& rstrCurrentAnimAction = cSelectedAnimations[nCurrentAnimation];
        if (!rstrCurrentAnimAction.isEmpty())
        {
            m_animation.m_animation = rstrCurrentAnimAction.toUtf8().data();
            m_animationLabel->setText(m_animation.m_animation.c_str());
            emit ValueChanged(m_animation);
        }
    }
}

QWidget* AnimationPropertyCtrl::GetFirstInTabOrder()
{
    return m_pApplyButton;
}
QWidget* AnimationPropertyCtrl::GetLastInTabOrder()
{
    return m_pApplyButton;
}

void AnimationPropertyCtrl::UpdateTabOrder()
{
    setTabOrder(m_pApplyButton, m_pApplyButton);
}


QWidget* AnimationPropertyWidgetHandler::CreateGUI(QWidget *pParent)
{
    AnimationPropertyCtrl* newCtrl = aznew AnimationPropertyCtrl(pParent);
    connect(newCtrl, &AnimationPropertyCtrl::ValueChanged, newCtrl, [newCtrl]()
    {
        EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
    });
    return newCtrl;
}


void AnimationPropertyWidgetHandler::ConsumeAttribute(AnimationPropertyCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    Q_UNUSED(GUI);
    Q_UNUSED(attrib);
    Q_UNUSED(attrValue);
    Q_UNUSED(debugName);
}

void AnimationPropertyWidgetHandler::WriteGUIValuesIntoProperty(size_t index, AnimationPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarAnimation val = GUI->value();
    instance = static_cast<property_t>(val);
}

bool AnimationPropertyWidgetHandler::ReadValuesIntoGUI(size_t index, AnimationPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarAnimation val = instance;
    GUI->SetValue(val);
    return false;
}


#include <Controls/ReflectedPropertyControl/moc_PropertyAnimationCtrl.cpp>


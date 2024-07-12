/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/BlendSpaceMotionHandler.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendSpaceManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceMotionPicker, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceMotionHandler, EditorAllocator)

    BlendSpaceMotionPicker::BlendSpaceMotionPicker(QWidget* parent)
        : QComboBox(parent)
        , m_blendSpaceNode(nullptr)
    {
        connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BlendSpaceMotionPicker::OnCurrentIndexChanged);
        ReInit();
    }


    void BlendSpaceMotionPicker::SetBlendSpaceNode(BlendSpaceNode* blendSpaceNode)
    {
        m_blendSpaceNode = blendSpaceNode;
        ReInit();
    }


    void BlendSpaceMotionPicker::ReInit()
    {
        clear();

        if (m_blendSpaceNode)
        {
            const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& motions = m_blendSpaceNode->GetMotions();
            for (const BlendSpaceNode::BlendSpaceMotion& motion : motions)
            {
                addItem(motion.GetMotionId().c_str());
            }
        }
    }


    void BlendSpaceMotionPicker::OnCurrentIndexChanged([[maybe_unused]] int index)
    {
        emit MotionChanged();
    }


    void BlendSpaceMotionPicker::SetMotionId(AZStd::string motionId)
    {
        setCurrentText(motionId.c_str());
    }


    AZStd::string BlendSpaceMotionPicker::GetMotionId() const
    {
        return currentText().toUtf8().data();
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    BlendSpaceMotionHandler::BlendSpaceMotionHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::string, BlendSpaceMotionPicker>()
        , m_blendSpaceNode(nullptr)
    {
    }


    AZ::u32 BlendSpaceMotionHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("BlendSpaceMotion");
    }


    QWidget* BlendSpaceMotionHandler::CreateGUI(QWidget* parent)
    {
        BlendSpaceMotionPicker* picker = aznew BlendSpaceMotionPicker(parent);

        connect(picker, &BlendSpaceMotionPicker::MotionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void BlendSpaceMotionHandler::ConsumeAttribute(BlendSpaceMotionPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrValue)
        {
            m_blendSpaceNode = static_cast<BlendSpaceNode*>(attrValue->GetInstance());
            GUI->SetBlendSpaceNode(m_blendSpaceNode);
        }

        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }


    void BlendSpaceMotionHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, BlendSpaceMotionPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetMotionId();
    }


    bool BlendSpaceMotionHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, BlendSpaceMotionPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetMotionId(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_BlendSpaceMotionHandler.cpp>

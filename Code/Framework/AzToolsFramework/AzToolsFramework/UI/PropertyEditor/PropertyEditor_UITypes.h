/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef PROPERTYEDITOR_UITYPES_H
#define PROPERTYEDITOR_UITYPES_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "PropertyEditor/EditorClassReflectionTest.h"

#pragma once

namespace AzToolsFramework
{
    namespace PropertySystem
    {
        typedef AZStd::function < void(const AZStd::string& FieldName, AZStd::vector<AZStd::string>& dEnumNames) >
            EnumNamesCallback;

        class EditorUIInfo_Enum
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Enum, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Enum, AZ::SystemAllocator, 0);

            EnumNamesCallback m_enumNamesCallBack;

            EditorUIInfo_Enum(EnumNamesCallback enumNamesCallBack, AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
                , m_enumNamesCallBack(enumNamesCallBack)
            {
            }
        };

        class EditorUIInfo_EnumComboBox
            : public EditorUIInfo_Enum
        {
        public:
            AZ_RTTI(EditorUIInfo_EnumComboBox, EditorUIInfo_Enum);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_EnumComboBox, AZ::SystemAllocator, 0);

            EditorUIInfo_EnumComboBox(EnumNamesCallback enumNamesCallBack, AZ::u32 inFlags = 0)
                : EditorUIInfo_Enum(enumNamesCallBack, inFlags)
            {
            }
        };

        typedef AZStd::function < void(const AZStd::string& FieldName, AZStd::vector<AZStd::string>& dEnumNames) >
            ChoiceNamesCallback;

        class EditorUIInfo_Choice
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Choice, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Choice, AZ::SystemAllocator, 0);

            ChoiceNamesCallback m_choiceNamesCallBack;

            EditorUIInfo_Choice(ChoiceNamesCallback choiceNamesCallBack, AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
                , m_choiceNamesCallBack(choiceNamesCallBack)
            {
            }
        };

        class EditorUIInfo_ChoiceComboBox
            : public EditorUIInfo_Choice
        {
        public:
            AZ_RTTI(EditorUIInfo_ChoiceComboBox, EditorUIInfo_Choice);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_ChoiceComboBox, AZ::SystemAllocator, 0);

            EditorUIInfo_ChoiceComboBox(ChoiceNamesCallback choiceNamesCallBack, AZ::u32 inFlags = 0)
                : EditorUIInfo_Choice(choiceNamesCallBack, inFlags)
            {
            }
        };

        class EditorUIInfo_Bool
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Bool, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Bool, AZ::SystemAllocator, 0);

            EditorUIInfo_Bool(AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
            {
            }
        };

        class EditorUIInfo_BoolComboBox
            : public EditorUIInfo_Bool
        {
        public:
            AZ_RTTI(EditorUIInfo_BoolComboBox, EditorUIInfo_Bool);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_BoolComboBox, AZ::SystemAllocator, 0);

            EditorUIInfo_BoolComboBox(AZ::u32 inFlags = 0)
                : EditorUIInfo_Bool(inFlags)
            {
            }
        };

        class EditorUIInfo_BoolDialogBox
            : public EditorUIInfo_Bool
        {
        public:
            AZ_RTTI(EditorUIInfo_BoolDialogBox, EditorUIInfo_Bool);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_BoolDialogBox, AZ::SystemAllocator, 0);

            EditorUIInfo_BoolDialogBox(AZ::u32 inFlags = 0)
                : EditorUIInfo_Bool(inFlags)
            {
            }
        };


        class EditorUIInfo_Int
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Int, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Int, AZ::SystemAllocator, 0);

            int m_minVal;
            int m_maxVal;

            EditorUIInfo_Int(int minVal = INT_MIN, int maxVal = INT_MAX, AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
                , m_minVal(minVal)
                , m_maxVal(maxVal)
            {
            }
        };

        class EditorUIInfo_IntSpinBox
            : public EditorUIInfo_Int
        {
        public:
            AZ_RTTI(EditorUIInfo_IntSpinBox, EditorUIInfo_Int);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_IntSpinBox, AZ::SystemAllocator, 0);

            EditorUIInfo_IntSpinBox(int minVal = INT_MIN, int maxVal = INT_MAX, AZ::u32 inFlags = 0)
                : EditorUIInfo_Int(minVal, maxVal, inFlags)
            {
            }
        };

        class EditorUIInfo_IntSlider
            : public EditorUIInfo_Int
        {
        public:
            AZ_RTTI(EditorUIInfo_IntSlider, EditorUIInfo_Int);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_IntSlider, AZ::SystemAllocator, 0);

            int m_step;

            EditorUIInfo_IntSlider(int step = 1, int minVal = INT_MIN, int maxVal = INT_MAX, AZ::u32 inFlags = 0)
                : EditorUIInfo_Int(minVal, maxVal, inFlags)
                , m_step(step)
            {
            }
        };

        class EditorUIInfo_Float
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Float, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Float, AZ::SystemAllocator, 0);

            float m_minVal;
            float m_maxVal;

            EditorUIInfo_Float(float minVal = std::numeric_limits<float>::min(), float maxVal = std::numeric_limits<float>::max(), AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
                , m_minVal(minVal)
                , m_maxVal(maxVal)
            {
            }
        };

        class EditorUIInfo_FloatSpinBox
            : public EditorUIInfo_Float
        {
        public:
            AZ_RTTI(EditorUIInfo_FloatSpinBox, EditorUIInfo_Float);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_FloatSpinBox, AZ::SystemAllocator, 0);

            EditorUIInfo_FloatSpinBox(float minVal = std::numeric_limits<float>::min(), float maxVal = std::numeric_limits<float>::max(), AZ::u32 inFlags = 0)
                : EditorUIInfo_Float(minVal, maxVal, inFlags)
            {
            }
        };

        class EditorUIInfo_FloatSlider
            : public EditorUIInfo_Float
        {
        public:
            AZ_RTTI(EditorUIInfo_FloatSlider, EditorUIInfo_Float);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_FloatSlider, AZ::SystemAllocator, 0);

            float m_step;

            EditorUIInfo_FloatSlider(float step = 1.f, float minVal = std::numeric_limits<float>::min(), float maxVal = std::numeric_limits<float>::max(), AZ::u32 inFlags = 0)
                : EditorUIInfo_Float(minVal, maxVal, inFlags)
                , m_step(step)
            {
            }
        };

        class EditorUIInfo_Double
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Double, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Double, AZ::SystemAllocator, 0);

            double m_minVal;
            double m_maxVal;

            EditorUIInfo_Double(double minVal = DBL_MIN, double maxVal = DBL_MAX, AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
                , m_minVal(minVal)
                , m_maxVal(maxVal)
            {
            }
        };

        class EditorUIInfo_DoubleSpinBox
            : public EditorUIInfo_Double
        {
        public:
            AZ_RTTI(EditorUIInfo_DoubleSpinBox, EditorUIInfo_Double);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_DoubleSpinBox, AZ::SystemAllocator, 0);

            EditorUIInfo_DoubleSpinBox(double minVal = DBL_MIN, double maxVal = DBL_MAX, AZ::u32 inFlags = 0)
                : EditorUIInfo_Double(minVal, maxVal, inFlags)
            {
            }
        };

        class EditorUIInfo_DoubleSlider
            : public EditorUIInfo_Double
        {
        public:
            AZ_RTTI(EditorUIInfo_DoubleSlider, EditorUIInfo_Double);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_DoubleSlider, AZ::SystemAllocator, 0);

            double m_step;

            EditorUIInfo_DoubleSlider(double step = 1.0, double minVal = DBL_MIN, double maxVal = DBL_MAX, AZ::u32 inFlags = 0)
                : EditorUIInfo_Double(minVal, maxVal, inFlags)
                , m_step(step)
            {
            }
        };

        class EditorUIInfo_String
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_String, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_String, AZ::SystemAllocator, 0);

            int m_maxChars;

            EditorUIInfo_String(int maxchars = -1, AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
                , m_maxChars(maxchars)
            {
            }
        };

        class EditorUIInfo_StringLineEdit
            : public EditorUIInfo_String
        {
        public:
            AZ_RTTI(EditorUIInfo_StringLineEdit, EditorUIInfo_String);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_StringLineEdit, AZ::SystemAllocator, 0);

            EditorUIInfo_StringLineEdit(int maxchars = -1, AZ::u32 inFlags = 0)
                : EditorUIInfo_String(maxchars, inFlags)
            {
            }
        };

        typedef AZStd::function<AZStd::string(int /*Index*/, int /*purpose*/)> DropListInfoCallback;

        class EditorUIInfo_DropdownList
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_DropdownList, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_DropdownList, AZ::SystemAllocator, 0);
            EditorUIInfo_DropdownList(DropListInfoCallback info, AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
            {
            }
        };

        // this function, if you supply it, will be called by the UI and other system to determine whether or not to show
        // your property at all.  This allows you to make properties which only show up when certain other properties are set.
        typedef AZStd::function < bool(const AZStd::string& /*property name*/, void* /* propertyOwner */, const EditorDataContext::ToolsComponentInfo* /* component info */) >
            GroupDisplayBooleanFunction;

        // a group is special in that it has children and uses a function to determine what to write for the group and whether to show the group
        class EditorUIInfo_Group
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Group, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Group, AZ::SystemAllocator, 0);
            EditorUIInfo_Group(GroupDisplayBooleanFunction displayBoolFn = 0, AZ::u32 inFlags = 0)
                : EditorUIInfo(inFlags)
                , m_displayBoolFn(displayBoolFn)
            {
            }
            GroupDisplayBooleanFunction m_displayBoolFn;
        };

        class EditorUIInfo_Class
            : public EditorUIInfo
        {
        public:
            AZ_RTTI(EditorUIInfo_Class, EditorUIInfo);
            AZ_CLASS_ALLOCATOR(EditorUIInfo_Class, AZ::SystemAllocator, 0);

            AZ::Uuid m_classID;

            EditorUIInfo_Class(const AZ::Uuid& classID = AZ::Uuid::CreateNull())
                : m_classID(classID)
            {
            }
        };
    }
} // namespace AzToolsFramework

#endif

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>

namespace TestNodes
{
    //////////////////////////////////////////////////////////////////////////////
    // TestResult node, prints a string to the console.
    class TestResult
        : public ScriptCanvas::Node
    {
    public:

        AZ_COMPONENT(TestResult, "{085CBDD3-D4E0-44D4-BF68-8732E35B9DF1}", ScriptCanvas::Node);
        
        static void Reflect(AZ::ReflectContext* context);

        // really only used for unit testing
        AZ_INLINE void SetText(const ScriptCanvas::Data::StringType& text) { m_string = text; }
        AZ_INLINE const ScriptCanvas::Data::StringType& GetText() const { return m_string; }

        void MarkDefaultableInput() override {}
                 
    protected:

        void OnInit() override;

    private:

        ScriptCanvas::Data::StringType m_string;
    };

    //////////////////////////////////////////////////////////////////////////////
    // ContractNode - used to test contract evaluation.
    class ContractNode : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(ContractNode, "{76A17F4F-F508-4C20-83A0-0125468946C7}", ScriptCanvas::Node);
        static void Reflect(AZ::ReflectContext* reflection);

        void OnInit() override;

    };

    //////////////////////////////////////////////////////////////////////////////
    class InfiniteLoopNode
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(InfiniteLoopNode, "{709A78D5-3449-4E94-B751-C68AC6385749}", Node);

        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        void OnInit() override;

    };

    //////////////////////////////////////////////////////////////////////////////
    class UnitTestErrorNode
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(UnitTestErrorNode, "{6A3E9EAD-F84B-4474-90B6-C3334DA669C2}", Node);

        static void Reflect(AZ::ReflectContext* reflection);

    protected:

        void OnInit() override;

    };

    //////////////////////////////////////////////////////////////////////////////
    class AddNodeWithRemoveSlot
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(AddNodeWithRemoveSlot, "{DE04D042-745F-4384-8B62-D6EE36321EFC}", ScriptCanvas::Node);

        static void Reflect(AZ::ReflectContext* reflection);
        ScriptCanvas::SlotId AddSlot(AZStd::string_view slotName);

        bool RemoveSlot(const ScriptCanvas::SlotId& slotId, bool emitWarning = true);

    protected:
        void OnInit() override;

        AZStd::vector<ScriptCanvas::SlotId> m_dynamicSlotIds;
        ScriptCanvas::SlotId m_resultSlotId;
    };

    //////////////////////////////////////////////////////////////////////////////
    class StringView
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(StringView, "{F47ACD24-79EB-4DBE-B325-8B9DB0839A75}", ScriptCanvas::Node);

    protected:

        void OnInit() override;

    public:

        static void Reflect(AZ::ReflectContext* context);

    private:
        ScriptCanvas::SlotId m_resultSlotId;
    };

    //////////////////////////////////////////////////////////////////////////////
    class InsertSlotConcatNode
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(InsertSlotConcatNode, "{445313E7-D0A5-4D73-B674-6FA37EFFF5C8}", ScriptCanvas::Node);

        static void Reflect(AZ::ReflectContext* reflection);
        ScriptCanvas::SlotId InsertSlot(AZ::s64 index, AZStd::string_view slotName);

    protected:
       void OnInit() override;
    };
    
    //////////////////////////////////////////////////////////////////////////////
    class ConfigurableUnitTestNode
        : public ScriptCanvas::Node
    {
    public:
        AZ_COMPONENT(ConfigurableUnitTestNode, "{61BF3D66-809A-4AAE-B20A-DFA2B51BE4EE}", ScriptCanvas::Node);
        static void Reflect(AZ::ReflectContext* reflection);

        ScriptCanvas::Slot* AddTestingSlot(const ScriptCanvas::SlotConfiguration& slotConfiguration);        
        ScriptCanvas::Slot* InsertTestingSlot(int index, const ScriptCanvas::SlotConfiguration& slotConfiguration);

        AZStd::vector< const ScriptCanvas::Slot* > FindSlotsByDescriptor(const ScriptCanvas::SlotDescriptor& slotDescriptor) const;

        void TestClearDisplayType(const AZ::Crc32& dynamicGroup);
        void TestSetDisplayType(const AZ::Crc32& dynamicGroup, const ScriptCanvas::Data::Type& dataType);
        bool TestHasConcreteDisplayType(const AZ::Crc32& dynamicGroup) const;

        bool TestIsSlotConnectedToConcreteDisplayType(const ScriptCanvas::Slot& slot, ExploredDynamicGroupCache& exploredGroupCache) const;

        void SetSlotExecutionMap(ScriptCanvas::SlotExecution::Map* executionMap);
        const ScriptCanvas::SlotExecution::Map* GetSlotExecutionMap() const override;

    private:
        ScriptCanvas::SlotExecution::Map* m_slotExecutionMap = nullptr;
    };


}

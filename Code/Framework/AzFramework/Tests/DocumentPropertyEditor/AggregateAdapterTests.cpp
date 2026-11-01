/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/AggregateAdapter.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    using AggregateAdapterDpeTests = DocumentPropertyEditorTestFixture;

    // ----------------------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------------------

    namespace
    {
        //! The contents of one component's adapter: a single labeled row holding a number.
        Dom::Value MakeRowContents(double value)
        {
            AdapterBuilder builder;
            builder.BeginAdapter();
            builder.BeginRow();
            builder.Label("Width");
            builder.BeginPropertyEditor("SpinBox", Dom::Value(value));
            builder.EndPropertyEditor();
            builder.EndRow();
            builder.EndAdapter();
            return builder.FinishAndTakeResult();
        }

        AZStd::shared_ptr<BasicAdapter> MakeSourceAdapter(double value)
        {
            auto adapter = AZStd::make_shared<BasicAdapter>();
            adapter->SetContents(MakeRowContents(value));
            return adapter;
        }

        //! A source adapter that only queues its reset when its value changes, as ReflectionAdapter::SetValue does.
        class QueuingAdapter : public DocumentAdapter
        {
        public:
            void SetValueQueued(double value)
            {
                m_value = MakeRowContents(value);
                QueueResetDocument(DocumentResetType::HardReset);
            }

        private:
            Dom::Value GenerateContents() override
            {
                return m_value;
            }

            Dom::Value m_value;
        };

        //! Counts what an adapter tells the views attached to it: reset events, and notifications that a reset is queued.
        struct ResetCounter
        {
            explicit ResetCounter(DocumentAdapter& adapter)
                : m_resetHandler([this]() { ++m_resets; })
                , m_queuedHandler([this]() { ++m_queued; })
            {
                adapter.ConnectResetHandler(m_resetHandler);
                adapter.ConnectResetQueuedHandler(m_queuedHandler);
            }

            int m_resets = 0;
            int m_queued = 0;
            DocumentAdapter::ResetEvent::Handler m_resetHandler;
            DocumentAdapter::ResetQueuedEvent::Handler m_queuedHandler;
        };

        //! The number in the property editor of the aggregate's first row.
        double FirstRowValue(const Dom::Value& contents)
        {
            const Dom::Value& row = contents[0];
            for (auto childIter = row.ArrayBegin(), endIter = row.ArrayEnd(); childIter != endIter; ++childIter)
            {
                if (childIter->GetNodeName() == GetNodeName<Nodes::PropertyEditor>())
                {
                    return Nodes::PropertyEditor::Value.ExtractFromDomNode(*childIter).value_or(Dom::Value(-1.0)).GetDouble();
                }
            }
            return -1.0;
        }

        //! The text of the label at labelIndex in the aggregate's first row, or empty.
        AZStd::string_view FirstRowLabel(const Dom::Value& contents, size_t labelIndex)
        {
            const Dom::Value& row = contents[0];
            size_t found = 0;
            for (auto childIter = row.ArrayBegin(), endIter = row.ArrayEnd(); childIter != endIter; ++childIter)
            {
                if (childIter->GetNodeName() == GetNodeName<Nodes::Label>() && found++ == labelIndex)
                {
                    return Nodes::Label::Value.ExtractFromDomNode(*childIter).value_or("");
                }
            }
            return {};
        }
    } // namespace

    // ----------------------------------------------------------------------------------
    // Adding adapters
    // ----------------------------------------------------------------------------------

    TEST_F(AggregateAdapterDpeTests, AddAdapter_QueuesOneResetForAllTheAdaptersOfAMultiSelection)
    {
        auto aggregate = AZStd::make_shared<LabeledRowAggregateAdapter>();
        ResetCounter view(*aggregate);

        aggregate->AddAdapter(MakeSourceAdapter(1.0));
        aggregate->AddAdapter(MakeSourceAdapter(1.0));
        aggregate->AddAdapter(MakeSourceAdapter(1.0));

        // adding rebuilds no view: the views are told that a reset is queued
        EXPECT_EQ(view.m_resets, 0);
        EXPECT_GE(view.m_queued, 1);

        // the view runs it once; a second run finds nothing queued
        aggregate->ExecuteQueuedReset();
        EXPECT_EQ(view.m_resets, 1);
        aggregate->ExecuteQueuedReset();
        EXPECT_EQ(view.m_resets, 1);
    }

    TEST_F(AggregateAdapterDpeTests, ExecuteQueuedReset_ShowsEveryAdapterAddedBeforeIt)
    {
        auto aggregate = AZStd::make_shared<LabeledRowAggregateAdapter>();
        aggregate->AddAdapter(MakeSourceAdapter(2.0));
        aggregate->AddAdapter(MakeSourceAdapter(2.0));
        aggregate->ExecuteQueuedReset();

        Dom::Value contents = aggregate->GetContents();
        ASSERT_EQ(contents.ArraySize(), 1);
        EXPECT_EQ(FirstRowLabel(contents, 0), "Width");
        EXPECT_EQ(FirstRowValue(contents), 2.0);

        // the last adapter added holds another value: the one reset shows the rows of all three, which now differ
        aggregate->AddAdapter(MakeSourceAdapter(3.0));
        aggregate->ExecuteQueuedReset();

        contents = aggregate->GetContents();
        ASSERT_EQ(contents.ArraySize(), 1);
        EXPECT_EQ(FirstRowLabel(contents, 0), "Width");
        EXPECT_EQ(FirstRowLabel(contents, 1), "Values Differ");
    }

    TEST_F(AggregateAdapterDpeTests, AddAdapter_ReadsTheCurrentValueOfASourceWhoseResetIsOnlyQueued)
    {
        auto source = AZStd::make_shared<QueuingAdapter>();
        source->SetValueQueued(1.0);
        source->ExecuteQueuedReset();
        ASSERT_FALSE(source->GetContents().IsNull()); // the source caches the contents of its previous value

        // a new value whose reset is only queued, as when a component editor is given another instance
        source->SetValueQueued(5.0);

        auto aggregate = AZStd::make_shared<LabeledRowAggregateAdapter>();
        aggregate->AddAdapter(source);
        aggregate->ExecuteQueuedReset();

        Dom::Value contents = aggregate->GetContents();
        ASSERT_EQ(contents.ArraySize(), 1);
        EXPECT_EQ(FirstRowValue(contents), 5.0);
    }

    // ----------------------------------------------------------------------------------
    // Clearing adapters
    // ----------------------------------------------------------------------------------

    TEST_F(AggregateAdapterDpeTests, ClearAdapters_ResetsTheViewsAtOnceToNoRows)
    {
        auto aggregate = AZStd::make_shared<LabeledRowAggregateAdapter>();
        aggregate->AddAdapter(MakeSourceAdapter(1.0));
        aggregate->AddAdapter(MakeSourceAdapter(1.0));
        aggregate->ExecuteQueuedReset();

        ResetCounter view(*aggregate);
        aggregate->ClearAdapters();

        // ComponentEditor::AddInstance relies on this when it leaves a multi-selection: the aggregate is detached before
        // the single adapter's own queued reset runs
        EXPECT_EQ(view.m_resets, 1);
        EXPECT_EQ(aggregate->GetContents().ArraySize(), 0);
    }
} // namespace AZ::DocumentPropertyEditor::Tests

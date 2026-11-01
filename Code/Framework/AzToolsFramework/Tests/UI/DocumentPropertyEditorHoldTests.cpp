/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DocumentPropertyEditor.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <QApplication>
#include <QEvent>

namespace UnitTest
{
    // ----------------------------------------------------------------------------------
    // A held DocumentPropertyEditor (the inspector's lazy cards): the adapter's resets, queued resets and changes build nothing while
    // it is held; releasing builds the rows once, from the adapter's current contents
    // ----------------------------------------------------------------------------------

    namespace DocumentPropertyEditorHoldTestsInternal
    {
        //! An adapter of labeled rows that counts how often it generates its contents
        class CountingAdapter : public AZ::DocumentPropertyEditor::DocumentAdapter
        {
        public:
            void SetLabels(AZStd::vector<AZStd::string> labels)
            {
                m_labels = AZStd::move(labels);
            }

            void Reset()
            {
                NotifyResetDocument(DocumentResetType::HardReset);
            }

            void QueueReset()
            {
                QueueResetDocument(DocumentResetType::HardReset);
            }

            //! The label of the first row becomes the text, in the contents and as a change the views are told of
            void ChangeFirstLabel(const AZStd::string& text)
            {
                m_labels[0] = text;
                AZ::Dom::Patch patch({ AZ::Dom::PatchOperation::ReplaceOperation(
                    AZ::Dom::Path("/0/0") / AZ::DocumentPropertyEditor::Nodes::Label::Value.GetName(), AZ::Dom::Value(text, true)) });
                NotifyContentsChanged(patch);
            }

            int m_generations = 0;

        protected:
            AZ::Dom::Value GenerateContents() override
            {
                ++m_generations;
                AZ::DocumentPropertyEditor::AdapterBuilder builder;
                builder.BeginAdapter();
                for (const AZStd::string& label : m_labels)
                {
                    builder.BeginRow();
                    builder.Label(label);
                    builder.EndRow();
                }
                builder.EndAdapter();
                return builder.FinishAndTakeResult();
            }

        private:
            AZStd::vector<AZStd::string> m_labels;
        };
    } // namespace DocumentPropertyEditorHoldTestsInternal

    class DocumentPropertyEditorHoldTests : public ToolsApplicationFixture<>
    {
    protected:
        void SetUpEditorFixtureImpl() override
        {
            m_adapter = AZStd::make_shared<DocumentPropertyEditorHoldTestsInternal::CountingAdapter>();
            m_adapter->SetLabels({ "Alpha", "Beta" });
            m_editor = AZStd::make_unique<AzToolsFramework::DocumentPropertyEditor>();
            m_editor->SetAdapter(m_adapter);    // builds the rows at once
        }

        void TearDownEditorFixtureImpl() override
        {
            m_editor.reset();
            m_adapter.reset();
        }

        //! The labels the editor shows, in order (the rows a clear released are deleted first)
        QStringList Shown() const
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QStringList labels;
            for (const AzQtComponents::ElidingLabel* label : m_editor->findChildren<AzQtComponents::ElidingLabel*>())
            {
                labels.push_back(label->text());
            }
            return labels;
        }

        AZStd::shared_ptr<DocumentPropertyEditorHoldTestsInternal::CountingAdapter> m_adapter;
        AZStd::unique_ptr<AzToolsFramework::DocumentPropertyEditor> m_editor;
    };

    TEST_F(DocumentPropertyEditorHoldTests, AHeldEditorBuildsNothingAndTheReleaseBuildsOnceFromTheCurrentContents)
    {
        ASSERT_EQ(Shown(), QStringList({ "Alpha", "Beta" }));
        const int generationsBefore = m_adapter->m_generations;

        m_editor->SetContentBuildHeld(true);
        m_adapter->SetLabels({ "Gamma", "Delta", "Epsilon" });
        m_adapter->Reset();
        m_adapter->QueueReset();
        QApplication::processEvents();      // a reset the editor posted for itself would run here

        EXPECT_EQ(Shown(), QStringList({ "Alpha", "Beta" }));
        EXPECT_TRUE(m_editor->IsContentBuildHeld());
        EXPECT_TRUE(m_editor->IsContentBuildPending());
        EXPECT_EQ(m_adapter->m_generations, generationsBefore);

        m_editor->SetContentBuildHeld(false);
        EXPECT_EQ(Shown(), QStringList({ "Gamma", "Delta", "Epsilon" }));
        EXPECT_EQ(m_adapter->m_generations, generationsBefore + 1);
        EXPECT_FALSE(m_editor->IsContentBuildHeld());
        EXPECT_FALSE(m_editor->IsContentBuildPending());

        // and nothing is left over to run once the events are processed
        QApplication::processEvents();
        EXPECT_EQ(m_adapter->m_generations, generationsBefore + 1);
    }

    TEST_F(DocumentPropertyEditorHoldTests, AChangeWhileHeldIsNotAppliedAndTheReleaseBuildsTheChangedContents)
    {
        m_editor->SetContentBuildHeld(true);
        m_adapter->ChangeFirstLabel("Changed");
        EXPECT_EQ(Shown(), QStringList({ "Alpha", "Beta" }));
        EXPECT_TRUE(m_editor->IsContentBuildPending());

        m_editor->SetContentBuildHeld(false);
        EXPECT_EQ(Shown(), QStringList({ "Changed", "Beta" }));
    }

    TEST_F(DocumentPropertyEditorHoldTests, AResetPostedBeforeTheHoldBuildsNothingWhenItRuns)
    {
        // the editor posts the execution of a queued reset to the event loop; the hold comes before the loop runs it
        m_adapter->SetLabels({ "Posted" });
        m_adapter->QueueReset();
        m_editor->SetContentBuildHeld(true);
        const int generationsBefore = m_adapter->m_generations;
        QApplication::processEvents();

        EXPECT_EQ(Shown(), QStringList({ "Alpha", "Beta" }));
        EXPECT_TRUE(m_editor->IsContentBuildPending());

        m_editor->SetContentBuildHeld(false);
        EXPECT_EQ(Shown(), QStringList({ "Posted" }));
        EXPECT_EQ(m_adapter->m_generations, generationsBefore + 1);
    }

    TEST_F(DocumentPropertyEditorHoldTests, ReleasingWithNothingPendingBuildsNothing)
    {
        const QList<AzQtComponents::ElidingLabel*> labelsBefore = m_editor->findChildren<AzQtComponents::ElidingLabel*>();
        const int generationsBefore = m_adapter->m_generations;

        m_editor->SetContentBuildHeld(true);
        m_editor->SetContentBuildHeld(false);

        EXPECT_EQ(m_editor->findChildren<AzQtComponents::ElidingLabel*>(), labelsBefore);
        EXPECT_EQ(m_adapter->m_generations, generationsBefore);
    }

    TEST_F(DocumentPropertyEditorHoldTests, DiscardingDropsThePendingBuildAndTheEditorFollowsItsAdapterAgain)
    {
        m_editor->SetContentBuildHeld(true);
        m_adapter->SetLabels({ "Dropped" });
        m_adapter->Reset();
        m_editor->DiscardHeldContentBuild();

        EXPECT_FALSE(m_editor->IsContentBuildHeld());
        EXPECT_FALSE(m_editor->IsContentBuildPending());
        EXPECT_EQ(Shown(), QStringList({ "Alpha", "Beta" }));

        m_adapter->SetLabels({ "Followed" });
        m_adapter->Reset();
        EXPECT_EQ(Shown(), QStringList({ "Followed" }));
    }
} // namespace UnitTest

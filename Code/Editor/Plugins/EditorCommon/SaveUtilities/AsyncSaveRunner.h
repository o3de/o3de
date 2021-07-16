#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/vector.h>
#include <EditorCommonAPI.h>

/*
------------------
AsyncSaveRunner:
------------------
== Overview ==
This class is meant to be a container for 1-n save operations that need to work with async source control commands. The asynchronous aspect of the SourceControlBus becomes
much more difficult when you have many operations because there is a management problem in knowing how soon until all commands have completed. This class will provide you
with an easy to use interface for specifying the save operations, and providing a single callback to be called once all those operations have been completed.

== Note ==
This class accepts lambdas and operates asynchronously. While the callbacks will be called on the main thread, **YOU MUST GUARANTEE LIFETIME YOURSELF**

== Usage ==
To use this class, you need to guarantee the lifetime of the save runner. The best way to do that is to store it as a member variable in the class that runs the save. Storing
it in a pointer type (or smart-pointer type) will help you control it's lifetime and memory.

Once you have a guaranteed lifetime AsyncSaveRunner, you'll build SaveOperationController instances that will manage all of your individual save operations, and will
run the source control pieces for you. This allows you to focus on specifying the pieces you care about.

This is easiest to see why you would want that, would be a 'Save All' scenario.
Lets imagine a save function that saves an item, which consists of saving a "header" file and an "entry" file:
    void SaveItem(int index)
    {
        auto item = m_items[index];

        auto controller = m_saveRunner->GenerateController();
        controller->AddSaveOperation(m_headerSaver.getPath(), [item](const AZStd::string& fullPath, const AZStd::shared_ptr<ActionOutput>& actionOutput)->bool
            {
                return item->headerSaver.save();
            }
        );

        controller->AddSaveOperation(m_entry.getPath(), [item](const AZStd::string& fullPath, const AZStd::shared_ptr<ActionOutput>& actionOutput)->bool
            {
                return item->entry.save();
            }
        );
    }

You can see that the AsyncSaveRunner was used to make a save operation controller, called 'controller' and that controller was filled out with save operations.
If it was desired, you could even add a callback per controller to know when each controller is finished (in case you have to run a custom notification or something
on the item).

You could imagine SaveItem being called 1-n times, adding more and more SaveOperationController instances to the AsyncSaveRunner. Once the runner is all filled out
you call run on it and pass it a callback. This callback will only be called once. This leaves our example to look like this:

    void SaveAll(AZStd::shared_ptr<AZ::ActionOutput> output, AZ::SaveCompleteCallback onComplete)
    {
        m_saveRunner = AZStd::make_shared<AZ::AsyncSaveRunner>();
        for(int index = 0; index < m_numItems; ++index)
        {
            Saveitem(index);
        }

        m_saveRunner->Run(output,
            [this](bool success)
            {
                m_saveRunner = nullptr;

                if(onComplete)
                {
                    onComplete(success);
                }
            }
        );
    }
*/

namespace AZ
{
    class ActionOutput;

    using SaveCompleteCallback = AZStd::function<void(bool success)>;
    using SynchronousSaveOperation = AZStd::function<bool(const AZStd::string& fullPath, const AZStd::shared_ptr<ActionOutput>& actionOutput)>;

    class AsyncSaveRunner;

    // Stores a cache of synchronous save operations, and runs them on completion of asynchronous source control operations.
    class EDITOR_COMMON_API SaveOperationController
    {
    public:
        explicit SaveOperationController(AsyncSaveRunner& owner);
        void AddSaveOperation(const AZStd::string& fullPath, SynchronousSaveOperation saveOperation);
        void AddDeleteOperation(const AZStd::string& fullPath, SynchronousSaveOperation saveOperation);
        void SetOnCompleteCallback(SaveCompleteCallback onThisRunnerComplete);

        void RunAll(const AZStd::shared_ptr<ActionOutput>& actionOutput);

        // Caches all synchronous save operations and associated data. Controlled by a SaveOperationController.
        class SaveOperationCache
        {
        public:
            SaveOperationCache(const AZStd::string& fullPath, SynchronousSaveOperation saveOperation, SaveOperationController& owner, bool isDelete = false);

            void Run(const AZStd::shared_ptr<ActionOutput>& actionOutput);
            void RunDelete(const AZStd::shared_ptr<ActionOutput>& actionOutput);
            friend class SaveOperationController;

        private:
            AZStd::string m_fullSavePath;
            SynchronousSaveOperation m_saveOperation;
            SaveOperationController& m_owner;
            bool m_isDelete;
        };

        void HandleOperationComplete(SaveOperationCache* saveOperation, bool success);

        friend class SaveRunner;

    private:
        AsyncSaveRunner& m_owner;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        AZStd::vector<AZStd::shared_ptr<SaveOperationCache>> m_allSaveOperations;
        SaveCompleteCallback m_onSaveComplete;
        AZStd::atomic<size_t> m_completedCount;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
        bool m_currentSaveResult = true;
    };

    // Builds, stores and executes SaveOperationController instances
    class EDITOR_COMMON_API AsyncSaveRunner
    {
    public:
        enum class ControllerOrder
        {
            // Random will run controllers at once and completion will happen randomly.
            Random,
            // Controllers are executed in order, waiting for one controller before starting
            //      the next one. Controllers internally will still have their executions
            //      complete in random order.
            Sequential
        };

        AZStd::shared_ptr<SaveOperationController> GenerateController();
        void Run(const AZStd::shared_ptr<ActionOutput>& actionOutput, SaveCompleteCallback onSaveAllComplete, ControllerOrder order);

    private:
        friend class SaveOperationController;
        void HandleRunnerFinished(SaveOperationController* runner, bool success);

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        AZStd::vector<AZStd::shared_ptr<SaveOperationController>> m_allSaveControllers;
        SaveCompleteCallback m_onSaveAllComplete;
        AZStd::shared_ptr<ActionOutput> m_actionOutput;
        // If controller order is random this keeps track of the number of completed tasks, if the order is sequential it
        //      keeps track of the currently executing controller.
        AZStd::atomic<size_t> m_counter;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
        ControllerOrder m_order;
        bool m_allWereSuccessfull = true;
    };
}

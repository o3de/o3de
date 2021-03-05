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
  
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Libraries/UnitTesting/UnitTestBus.h>

namespace ScriptCanvas
{
    class RuntimeComponent;

    namespace UnitTesting
    {
        class Reporter
            : public Bus::Handler
            , public AZ::EntityBus::Handler
        {
        public:
            Reporter();
            Reporter(const RuntimeComponent& graph);
            ~Reporter();

            void FinishReport();
            
            void FinishReport(const RuntimeComponent& graph);
            
            const AZStd::vector<Report>& GetCheckpoints() const;

            const AZStd::vector<Report>& GetFailure() const;

            const AZ::EntityId& GetGraphId() const;
            
            const AZStd::vector<Report>& GetSuccess() const;
                        
            bool IsActivated() const;
            
            bool IsComplete() const;

            bool IsDeactivated() const;
            
            bool IsErrorFree() const;

            bool IsReportFinished() const;

            bool operator==(const Reporter& other) const;
            
            void Reset();

            void SetGraph(const RuntimeComponent& graph);

            // Bus::Handler
            
            void AddFailure(const Report& report) override;

            void AddSuccess(const Report& report) override;

            void Checkpoint(const Report& report) override;

            void ExpectFalse(const bool value, const Report& report) override;

            void ExpectTrue(const bool value, const Report& report) override;

            void MarkComplete(const Report& report) override;
            
            SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_OVERRIDES(ExpectEqual);
                        
            SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_OVERRIDES(ExpectNotEqual);

            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectGreaterThan);

            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectGreaterThanEqual);

            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectLessThan);

            SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectLessThanEqual);
        
        protected:
            void OnEntityActivated(const AZ::EntityId&) override;
            void OnEntityDeactivated(const AZ::EntityId&) override;

        private:
            bool m_graphIsActivated = false;
            bool m_graphIsDeactivated = false;
            bool m_graphIsComplete = false;
            bool m_graphIsErrorFree = false;
            bool m_isReportFinished = false;
            AZ::EntityId m_graphId;
            AZ::EntityId m_entityId;
            AZStd::vector<Report> m_checkpoints;
            AZStd::vector<Report> m_failures;
            AZStd::vector<Report> m_successes;            
        }; // class Reporter/

    } // namespace UnitTesting

} // namespace ScriptCanvas

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/TraceReflection.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>

namespace AZ
{
    namespace Debug
    {
        class TraceMessageBusHandler
            : public AZ::Debug::TraceMessageBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:

            AZ_EBUS_BEHAVIOR_BINDER(TraceMessageBusHandler, "{5CDBAF09-5EB0-48AC-B327-2AF8601BB550}", AZ::SystemAllocator
                , OnPreAssert
                , OnPreError
                , OnPreWarning
                , OnAssert
                , OnError
                , OnWarning
                , OnException
                , OnPrintf
                , OnOutput
            );

            // TraceMessageBus
            /*
             *  Note: Since at editor runtime there is already have a handler, for automation (OnPreAssert, OnPreWarning, OnPreWarning)
             *  must be used instead of (OnAssert, OnWarning, OnError)
            */
            bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
            bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
            bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
            bool OnAssert(const char* message) override;
            bool OnError(const char* window, const char* message) override;
            bool OnWarning(const char* window, const char* message) override;
            bool OnException(const char* message) override;
            bool OnPrintf(const char* window, const char* message) override;
            bool OnOutput(const char* window, const char* message) override;

        private:
            template<class R, class... Args>
            R CallResultReturn(const R& defaultReturnValue, int index, Args&&... args) const
            {
                R returnVal = defaultReturnValue;
                CallResult(returnVal, index, AZStd::forward<Args>(args)...);
                return returnVal;
            }
        };

        //////////////////////////////////////////////////////////////////////////
        // TraceMessageBusHandler Implementation
        inline bool TraceMessageBusHandler::OnPreAssert(const char* fileName, int line, const char* func, const char* message)
        {
            return CallResultReturn(false, FN_OnPreAssert, fileName, line, func, message);
        }

        inline bool TraceMessageBusHandler::OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            return CallResultReturn(false, FN_OnPreError, window, fileName, line, func, message);
        }

        inline bool TraceMessageBusHandler::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            return CallResultReturn(false, FN_OnPreWarning, window, fileName, line, func, message);
        }

        inline bool TraceMessageBusHandler::OnAssert(const char* message)
        {
            return CallResultReturn(false, FN_OnAssert, message);
        }

        inline bool TraceMessageBusHandler::OnError(const char* window, const char* message)
        {
            return CallResultReturn(false, FN_OnError, window, message);
        }

        inline bool TraceMessageBusHandler::OnWarning(const char* window, const char* message)
        {
            return CallResultReturn(false, FN_OnWarning, window, message);
        }

        inline bool TraceMessageBusHandler::OnException(const char* message)
        {
            return CallResultReturn(false, FN_OnException, message);
        }

        inline bool TraceMessageBusHandler::OnPrintf(const char* window, const char* message)
        {
            return CallResultReturn(false, FN_OnPrintf, window, message);
        }

        inline bool TraceMessageBusHandler::OnOutput(const char* window, const char* message)
        {
            return CallResultReturn(false, FN_OnOutput, window, message);
        }


        void TraceReflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<AZ::Debug::TraceMessageBus>("TraceMessageBus")
                    ->Attribute(AZ::Script::Attributes::Module, "debug")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Handler<TraceMessageBusHandler>()
                    ;
            }
        }
    }
}

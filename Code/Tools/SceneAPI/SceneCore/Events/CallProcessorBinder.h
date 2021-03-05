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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            // CallProcessorBinder automatically registers to the CallProcessorBus to
            //      handle process calls on behave to the parent class by filtering
            //      and forwarding calls to the appropriate functions.
            //      To use, derive from CallProcessorBinder and call "BindToCall"
            //      one or more times to register functions with that accept a
            //      processor context with the signature "ProcessingResult(X& context) const" or 
            //      "ProcessingResult(X& context)", where X is any class derived from ICallContext.
            //
            // Example:
            //      Example inherits from CallProcessorBinder and has the following
            //      function: ProcessingResult ProcessContext(ExampleContext& context);
            //      In Example's constructor call:
            //          BindToCall(&Example::ProcessContext);
            //      If an processor call with the ExampleContext is send,
            //      Example::ProcessContext will automatically be called.
            class SCENE_CORE_CLASS CallProcessorBinder :
                public CallProcessorBus::Handler
            {
            public:
                enum class TypeMatch
                {
                    Exact,
                    Derived
                };

                AZ_RTTI(CallProcessorBinder, "{887A50B4-3FC4-4695-A88E-CA7BE931A73E}");
                SCENE_CORE_API ProcessingResult Process(ICallContext* context) override final;

                CallProcessorBinder() = default;
                SCENE_CORE_API virtual ~CallProcessorBinder();

            protected:
                CallProcessorBinder(const CallProcessorBinder&) = delete;

                template<typename Class, typename ContextType>
                inline void BindToCall(ProcessingResult(Class::*Func)(ContextType& context) const, TypeMatch typeMatch = TypeMatch::Exact);
                template<typename Class, typename ContextType>
                inline void BindToCall(ProcessingResult(Class::*Func)(ContextType& context), TypeMatch typeMatch = TypeMatch::Exact);

                SCENE_CORE_API void ActivateBindings();
                SCENE_CORE_API void DeactivateBindings();
                SCENE_CORE_API void ClearBindings();

            private:
                class FunctionBinding
                {
                public:
                    virtual ~FunctionBinding() = default;
                    virtual ProcessingResult Process(CallProcessorBinder* thisPtr, ICallContext* context) = 0;

                protected:
                    template<typename Class, typename ContextType, typename Function>
                    ProcessingResult Call(CallProcessorBinder* thisPtr, ICallContext* context, Function function);
                };

                template<typename Class, typename ContextType>
                class ConstFunctionBindingTemplate : public FunctionBinding
                {
                public:
                    using Function = ProcessingResult(Class::*)(ContextType&) const;
                    explicit ConstFunctionBindingTemplate(Function function);
                    ~ConstFunctionBindingTemplate() override = default;
                    ProcessingResult Process(CallProcessorBinder* thisPtr, ICallContext* context) override;
                private:
                    Function m_function;
                };

                template<typename Class, typename ContextType>
                class FunctionBindingTemplate : public FunctionBinding
                {
                public:
                    using Function = ProcessingResult(Class::*)(ContextType&);
                    explicit FunctionBindingTemplate(Function function);
                    ~FunctionBindingTemplate() override = default;
                    ProcessingResult Process(CallProcessorBinder* thisPtr, ICallContext* context) override;
                private:
                    Function m_function;
                };

                template<typename Class, typename ContextType>
                class ConstDerivedFunctionBindingTemplate : public FunctionBinding
                {
                public:
                    using Function = ProcessingResult(Class::*)(ContextType&) const;
                    explicit ConstDerivedFunctionBindingTemplate(Function function);
                    ~ConstDerivedFunctionBindingTemplate() override = default;
                    ProcessingResult Process(CallProcessorBinder* thisPtr, ICallContext* context) override;
                private:
                    Function m_function;
                };

                template<typename Class, typename ContextType>
                class DerivedFunctionBindingTemplate : public FunctionBinding
                {
                public:
                    using Function = ProcessingResult(Class::*)(ContextType&);
                    explicit DerivedFunctionBindingTemplate(Function function);
                    ~DerivedFunctionBindingTemplate() override = default;
                    ProcessingResult Process(CallProcessorBinder* thisPtr, ICallContext* context) override;
                private:
                    Function m_function;
                };

                AZStd::vector<AZStd::unique_ptr<FunctionBinding>> m_bindings;
            };
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ

#include <SceneAPI/SceneCore/Events/CallProcessorBinder.inl>
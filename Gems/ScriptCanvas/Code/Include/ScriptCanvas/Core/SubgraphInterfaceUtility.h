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

#include "SubgraphInterface.h"

namespace AZ
{
    class BehaviorContext;
    class BehaviorMethod;
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        bool IsDefaultOutId(const FunctionSourceId& id);

        FunctionSourceId MakeDefaultOutId(const FunctionSourceId& inID);

        bool OutIdIsEqual(const FunctionSourceId& lhs, const FunctionSourceId& rhs);

        Ins CreateInsFromBehaviorContextMethods(const AZStd::string& className, const AZ::BehaviorContext& behaviorContext, const AZStd::unordered_set<AZStd::string>& branchingMethods);
     
        Outs CreateOutsFromBehaviorContextMethod(const AZ::BehaviorMethod& method);

        template<typename T>
        Input CreateInput(const AZStd::string& name)
        {
            Input input;
            SetDisplayAndParsedName(input, name);
            input.datum = Datum(Data::FromAZType<T>(), Datum::eOriginality::Original);
            return input;
        }

        template<typename... t_Args, AZStd::size_t... Is>
        In CreateInHelper(const AZStd::string& name, const AZStd::vector<AZStd::string>& InputNames, AZStd::index_sequence<Is...>)
        {
            In in;
            in.displayName = name;
            in.parsedName = name;
            in.inputs.reserve(sizeof...(Is));

            int dummy[]{ 0, (in.inputs.emplace_back(CreateInput<t_Args>(InputNames[Is])), 0)... };
            static_cast<void>(dummy); /* avoid warning for unused variable */

            return in;
        }

        template<typename... t_Args>
        In CreateIn(const AZStd::string& name, const AZStd::vector<AZStd::string>& InputNames = {})
        {
            return CreateInHelper<t_Args...>(name, InputNames, AZStd::make_index_sequence<sizeof...(t_Args)>());
        }

        template<typename T>
        Output CreateOutput(const AZStd::string& name)
        {
            Output output;
            SetDisplayAndParsedName(output, name);
            output.type = Data::FromAZType<T>();
            return output;
        }
        
        template<typename... t_Args, AZStd::size_t... Is>
        Out CreateOutHelper(const AZStd::string& name, const AZStd::vector<AZStd::string>& outputNames, AZStd::index_sequence<Is...>)
        {
            Out out;
            SetDisplayAndParsedName(out, name);
            out.outputs.reserve(sizeof...(Is));

            int dummy[]{ 0, (out.outputs.emplace_back(CreateOutput<t_Args>(outputNames[Is])), 0)... };
            static_cast<void>(dummy); /* avoid warning for unused variable */

            return out;
        }

        template<typename... t_Args>
        Out CreateOut(const AZStd::string& name, const AZStd::vector<AZStd::string>& outputNames = {})
        {
            return CreateOutHelper<t_Args...>(name, outputNames, AZStd::make_index_sequence<sizeof...(t_Args)>());
        }


        template<typename t_Return, typename... t_Args, AZStd::size_t... Is>
        Out CreateOutReturnHelper(const AZStd::string& name, const AZStd::string& returnName, const AZStd::vector<AZStd::string>& outputNames, AZStd::index_sequence<Is...>)
        {
            Out out;
            SetDisplayAndParsedName(out, name);
            out.returnValues.push_back(CreateInput<t_Return>(returnName));
            out.outputs.reserve(sizeof...(Is));

            int dummy[]{ 0, (out.outputs.emplace_back(CreateOutput<t_Args>(outputNames[Is])), 0)... };
            static_cast<void>(dummy); /* avoid warning for unused variable */

            return out;
        }

        template<typename t_Return, typename... t_Args>
        Out CreateOutReturn(const AZStd::string& name, const AZStd::string& returnName, const AZStd::vector<AZStd::string>& outputNames = {})
        {
            return CreateOutReturnHelper<t_Return, t_Args...>(name, returnName, outputNames, AZStd::make_index_sequence<sizeof...(t_Args)>());
        }

        In* FindInByName(const AZStd::string& in, Ins& ins);
        In* FindInByNameNoError(const AZStd::string& in, Ins& ins);


    } 
    
} 

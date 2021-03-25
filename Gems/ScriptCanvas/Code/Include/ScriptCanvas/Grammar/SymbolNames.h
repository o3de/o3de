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

// children: [0]
REGISTER_ENUM(Break)
// children: [1]
REGISTER_ENUM(CompareEqual)
// children: [1]
REGISTER_ENUM(CompareGreater)
// children: [1]
REGISTER_ENUM(CompareGreaterEqual)
// children: [1]
REGISTER_ENUM(CompareLess)
// children: [1]
REGISTER_ENUM(CompareLessEqual)
// children: [1]
REGISTER_ENUM(CompareNotEqual)
// children: [1, n), all lead to execution leaves (just like a switch statement)
REGISTER_ENUM(Cycle)
// children: [0]
REGISTER_ENUM(DebugInfoEmptyStatement)
// children: [2], loop, finished, finished leads to execution leaf
REGISTER_ENUM(ForEach)
// children: [1]
REGISTER_ENUM(FunctionCall)
// children: [0]
REGISTER_ENUM(FunctionDefinition)
// children: [1,2), all lead to execution leaves
REGISTER_ENUM(IfCondition)
// children: [1,2), all lead to execution leaves 
REGISTER_ENUM(IsNull)
// children: [1]
REGISTER_ENUM(LogicalAND)
// children: [1]
REGISTER_ENUM(LogicalNOT)
// children: [1]
REGISTER_ENUM(LogicalOR)
// children: [1]
REGISTER_ENUM(OperatorAddition)
// children: [1]
REGISTER_ENUM(OperatorDivision)
// children: [1]
REGISTER_ENUM(OperatorMultiplication)
// children: [1]
REGISTER_ENUM(OperatorSubraction)
// children: [1]
REGISTER_ENUM(PlaceHolderDuringParsing)
// children: [1, n), all lead to execution leaves
REGISTER_ENUM(RandomSwitch)
// children: [1, n), nth child leads to leaf
REGISTER_ENUM(Sequence)
// children: [1, n), all lead to execution leaves
REGISTER_ENUM(Switch)
// children: [1]
REGISTER_ENUM(UserOut)
// children: [1]
REGISTER_ENUM(VariableAssignment)
// children: [1]
REGISTER_ENUM(VariableDeclaration)
// children: [2], loop, finished, finished leads to execution leaf
REGISTER_ENUM(While)

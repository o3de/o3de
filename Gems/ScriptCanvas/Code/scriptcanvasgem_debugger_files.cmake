#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/ScriptCanvas/Debugger/API.h
    Include/ScriptCanvas/Debugger/API.cpp
    Include/ScriptCanvas/Debugger/APIArguments.h
    Include/ScriptCanvas/Debugger/APIArguments.cpp
    Include/ScriptCanvas/Debugger/Bus.h
    Include/ScriptCanvas/Debugger/ClientTransceiver.h
    Include/ScriptCanvas/Debugger/ClientTransceiver.cpp
    Include/ScriptCanvas/Debugger/Debugger.h
    Include/ScriptCanvas/Debugger/Debugger.cpp
    Include/ScriptCanvas/Debugger/Logger.h
    Include/ScriptCanvas/Debugger/Logger.cpp
    Include/ScriptCanvas/Debugger/LogReader.h
    Include/ScriptCanvas/Debugger/LogReader.cpp
    Include/ScriptCanvas/Debugger/StatusBus.h
    Include/ScriptCanvas/Debugger/Messages/Notify.cpp
    Include/ScriptCanvas/Debugger/Messages/Notify.h
    Include/ScriptCanvas/Debugger/Messages/Request.cpp
    Include/ScriptCanvas/Debugger/Messages/Request.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/GraphTranslationValidation/GraphTranslationValidations.h
    Include/ScriptCanvas/Debugger/ValidationEvents/GraphTranslationValidation/GraphTranslationValidationIds.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidations.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidationIds.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationEvents.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/DynamicDataTypeEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/InvalidExpressionEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/InvalidRandomSignalEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/ScopedDataConnectionEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/SlotReferenceEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/UnknownEndpointEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/ScriptEventVersionMismatch.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ExecutionValidation/ExecutionValidationEvents.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ExecutionValidation/ExecutionValidationIds.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ExecutionValidation/UnusedNodeEvent.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h
    Include/ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/GreyOutEffect.h
)

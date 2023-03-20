/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace ScriptCanvas
{
    namespace Execution
    {
        // This Lua chunk provides the required class system support with Lua defined functions to provide SC users with inheritance for
        // any object and the ability to execute user defined Nodeables.
        constexpr const char* k_LuaClassInheritanceChunk =
            R"LUA(
if _G.SCRIPT_CANVAS_GLOBAL_RELEASE then

function _G.GetExecutionOutClass_SCVM(self, index)
    return self.executionsOut[index]
end

function _G.InitializeExecutionOutsClass_SCVM(self, count)
    self.executionsOut = {}

    for i=1,count do
        table.insert(self.executionsOut, ExecutionOutClassInitializer_SCVM)
    end
end

function _G.SetExecutionOutClass_SCVM(self, index, executionOut)
    self.executionsOut[index] = executionOut
end

function _G.ExecutionOutClassInitializer_SCVM()
    -- empty, this is just to reserve space in the out table
end

else -- not release

function _G.GetExecutionOutClass_SCVM(self, index)
    assert(type(self.executionsOut) == 'table', 'GetExecutionOutClass_SCVM in '..tostring(self.className_SCVM)..' executionsOut was not initialized to a table')
    assert(type(self.executionsOut[index]) == 'function', tostring(self.className_SCVM)..' executionsOut['..tostring(index)..'] was not initialized to a function')
    return self.executionsOut[index]
end

function _G.InitializeExecutionOutsClass_SCVM(self, count)
    assert(type(self.executionsOut) == 'nil', 'InitializeExecutionOuts in '..tostring(self.className_SCVM)..' executionsOut was not initialized to nil')
    assert(count > 0, 'InitializeExecutionOuts in '..tostring(self.className_SCVM)..' count < 1')
    self.executionsOut = {}

    for i=1,count do
        table.insert(self.executionsOut, ExecutionOutClassInitializer_SCVM)
    end
end

function _G.SetExecutionOutClass_SCVM(self, index, executionOut)
    assert(type(self.executionsOut) == 'table', 'SetExecutionOut in '..tostring(self.className_SCVM)..' executionsOut was not initialized to a table')
    assert(type(executionOut == 'function', 'SetExecutionOut in '..tostring(self.className_SCVM)..' received an argument that was not a function: '..type(executionOut)))
    self.executionsOut[index] = executionOut
end

function _G.ExecutionOutClassInitializer_SCVM()
    -- empty, this is just to reserve space in the out table
end

end
)LUA";
    }
}

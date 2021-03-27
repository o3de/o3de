"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
print ('Testing framework Python bindings')

import azlmbr.entity as entity
import azlmbr.framework as framework
import azlmbr.math as math

def test_entity_api():
    entityId = entity.EntityId()

    if (entityId.IsValid is None):
        print('entityId.IsValid is None')
        framework.Terminate(1) 

    if (entityId.IsValid() is True):
        print('entityId.IsValid() is True')
        framework.Terminate(2)

def test_math_api():
    if (math.Math_IsEven(3)):
        framework.Terminate(1) 

    if (math.Math_IsClose(1.0,2.0)):
        framework.Terminate(2) 

    if (math.Math_IsClose(2.0, math.Math_Sqrt(4.0)) is False):
        framework.Terminate(3) 

def main():
    import sys
    print("Starting framework test")
    if (sys.argv[1] == 'entity'):
        test_entity_api()
    elif (sys.argv[1] == 'math'):
        test_math_api()
    else:
        print('Unknown arg {}'.format(sys.argv[0]))
        framework.Terminate(4)

if __name__ == "__main__":
    main()
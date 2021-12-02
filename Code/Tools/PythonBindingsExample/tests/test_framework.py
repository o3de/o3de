"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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

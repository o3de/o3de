# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
# send each line to maya with > send_py_cmd_to_maya
print('Hello World: Command received from WingIDE')
import maya.cmds as cmds
foo = cmds.polySphere(n='DemoSphere', radius=1.0)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# more complex example
import maya.cmds as cmds
import random 
import time

name = 'DemoCube'
size = random.uniform(0.5, 2.0)
variation = random.uniform(1.5, 5.0)
amount = random.randint(9, 21)

# remove previous
obj_list = cmds.ls('{}*'.format(name))
if len(obj_list) > 0:
    cmds.delete(obj_list)

for i in range(0, amount - 1):

    depth_rand = random.uniform(size, size * variation)

    tegel = cmds.polyCube(name='{}#'.format(name),
                          w=size, h=size, d=depth_rand)
    cmds.move(size * i, 0, 5)
    i += 1
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
import maya.cmds as cmds
import random 
import time

name = 'DemoCube'
size = random.uniform(0.5, 2.0)
variation = random.uniform(1.5, 5.0)
amount = random.randint(9, 21)

def make_some_wonky_cubes(name, size, variation, amount):
    # remove previous
    obj_list = cmds.ls('{}*'.format(name))
    if len(obj_list) > 0:
        cmds.delete(obj_list)

    for i in range(0, amount - 1):

        depth_rand = random.uniform(size, size * variation)

        tegel = cmds.polyCube(name='{}#'.format(name),
                              w=size, h=size, d=depth_rand)
        cmds.move(size * i, 0, 5)
        i += 1
    return

foo = make_some_wonky_cubes()
time.sleep(10)
foo = make_some_wonky_cubes()

while 1:
    foo = make_some_wonky_cubes()
    time.sleep(5)
    foo = make_some_wonky_cubes(size=0.5, amount=20)
    time.sleep(5)
    break

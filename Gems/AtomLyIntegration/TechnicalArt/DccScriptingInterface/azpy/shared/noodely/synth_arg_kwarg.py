"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# synth_arg_kwarg.py
# Convenience module for a standardized attr interface for classes/objects.
# -------------------------------------------------------------------------
__author__ = 'HogJonny'
# -------------------------------------------------------------------------
from find_arg import find_arg
from synth import synthesize

# -------------------------------------------------------------------------


def setSynthArgKwarg(inst, argPosIndex=None, argTag=None, defaultValue=None,
                     inArgs=None, inKwargs=None, removeKwarg=True,
                     setAnyway=True):
    """
    Uses find_arg and sets a property on a object.

    Special args:
        setAnyway  <-- if the object has the property already, set it

    If the arg/property doesn't exist we synthesize it
    """

    foundArg = None
    argValueDict = {}

    # find the argument, or set to default value
    foundArg, inKwargs = find_arg(argPosIndex, argTag, removeKwarg,
                                  inArgs, inKwargs,
                                  defaultValue)

    if foundArg:
        argTag = foundArg

    # single arg first
    # make sure the object doesn't arealdy have this property
    try:
        hasattr(inst, argTag)  # check if property exists
        if setAnyway:
            try:
                setattr(inst, argTag, defaultValue)  # try to set
            except Exception as e:
                raise e
    except:
        pass

    # make it a synthetic property
    if argTag:
        try:
            argValue = synthesize(inst, argTag, defaultValue)
            argValueDict[argTag] = argValue
        except Exception as e:
            raise e

    # multiple and/or remaining kwards next
    if inKwargs:
        if len(inKwargs) > 0:
            for k, v in inKwargs.items():
                try:
                    hasattr(inst, k)  # check if property exists
                    if setAnyway:
                        try:
                            setattr(inst, k, v)  # try to set
                        except Exception as e:
                            raise e
                except:
                    pass

                if k:
                    try:
                        argValue = synthesize(inst, k, v)
                        argValueDict[k] = argValue
                    except Exception as e:
                        raise e

    return argValueDict
# --------------------------------------------------------------------------


###########################################################################
# Main Code Block, will run the tool
# -------------------------------------------------------------------------
if __name__ == '__main__':

    from test_foo import Foo

    # define a arg/property tag we know doesn't exist
    synthArgTag = 'syntheticArg'

    # create a test object
    print('~ creating the test foo object...')
    myFoo = Foo()

    print('~ Starting - single synthetic arg test...')
    # find and set existing, or create and set
    argValueDict = setSynthArgKwarg(myFoo,
                                    argTag=synthArgTag,
                                    defaultValue='kablooey')

    # what was returned
    print('~ single value returned...')
    for k, v in argValueDict.items():
        print("Arg '{0}':'{1}'".format(k, v))

    # attempt to access the new synthetic property directly
    print('~ direct property access test...')
    try:
        myFoo.syntheticArg
        print('myFoo.{0}: {1}'.format(synthArgTag, myFoo.syntheticArg))
    except Exception as e:
        raise e

    # can we create a bunch of kwargs?
    print('~ Starting - multiple synthetic kwarg test...')
    newKwargs = {'fooey': 'chop suey', 'success': True}

    # find and set existing, or create and set
    argValueDict = setSynthArgKwarg(myFoo,
                                    inKwargs=newKwargs,
                                    defaultValue='kablooey')

    # what was returned
    print('~ multiple values returned...')
    for k, v in argValueDict.items():
        print("Arg '{0}':'{1}'".format(k, v))

    print('~ multiple direct property access test...')
    try:
        myFoo.fooey
        print('myFoo.{0}: {1}'.format('fooey', myFoo.fooey))
    except Exception as e:
        raise e

    try:
        myFoo.success
        print('myFoo.{0}: {1}'.format('success', myFoo.success))
    except Exception as e:
        raise e

    print('~ Starting - known failure test...')
    try:
        myFoo.knownBad
    except Exception as e:
        print(e)
        print('Test failed as expected!!!')

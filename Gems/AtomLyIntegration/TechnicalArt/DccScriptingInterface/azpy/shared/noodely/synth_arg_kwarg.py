"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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


def set_synth_arg_kwarg(inst, arg_pos_index=None, arg_tag=None, default_value=None,
                     in_args=None, in_kwargs=None, remove_kwarg=True,
                     set_anyway=True):
    """
    Uses find_arg and sets a property on a object.

    Special args:
        setAnyway  <-- if the object has the property already, set it

    If the arg/property doesn't exist we synthesize it
    """

    found_arg = None
    arg_value_dict = {}

    # find the argument, or set to default value
    found_arg, in_kwargs = find_arg(arg_pos_index, arg_tag, remove_kwarg,
                                  in_args, in_kwargs,
                                  default_value)

    if found_arg:
        arg_tag = found_arg

    # single arg first
    # make sure the object doesn't arealdy have this property
    try:
        hasattr(inst, arg_tag)  # check if property exists
        if set_anyway:
            try:
                setattr(inst, arg_tag, default_value)  # try to set
            except Exception as e:
                raise e
    except:
        pass

    # make it a synthetic property
    if arg_tag:
        try:
            arg_value = synthesize(inst, arg_tag, default_value)
            arg_value_dict[arg_tag] = arg_value
        except Exception as e:
            raise e

    # multiple and/or remaining kwards next
    if in_kwargs:
        if len(in_kwargs) > 0:
            for k, v in in_kwargs.items():
                try:
                    hasattr(inst, k)  # check if property exists
                    if set_anyway:
                        try:
                            setattr(inst, k, v)  # try to set
                        except Exception as e:
                            raise e
                except:
                    pass

                if k:
                    try:
                        arg_value = synthesize(inst, k, v)
                        arg_value_dict[k] = arg_value
                    except Exception as e:
                        raise e

    return arg_value_dict
# --------------------------------------------------------------------------


###########################################################################
# Main Code Block, will run the tool
# -------------------------------------------------------------------------
if __name__ == '__main__':

    from test_foo import Foo

    # define a arg/property tag we know doesn't exist
    synth_arg_tag = 'synthetic_arg'

    # create a test object
    print('~ creating the test foo object...')
    my_foo = Foo()

    print('~ Starting - single synthetic arg test...')
    # find and set existing, or create and set
    arg_value_dict = set_synth_arg_kwarg(my_foo,
                                    arg_tag=synth_arg_tag,
                                    default_value='kablooey')

    # what was returned
    print('~ single value returned...')
    for k, v in arg_value_dict.items():
        print("Arg '{0}':'{1}'".format(k, v))

    # attempt to access the new synthetic property directly
    print('~ direct property access test...')
    try:
        my_foo.synthetic_arg
        print('myFoo.{0}: {1}'.format(synth_arg_tag, my_foo.synthetic_arg))
    except Exception as e:
        raise e

    # can we create a bunch of kwargs?
    print('~ Starting - multiple synthetic kwarg test...')
    newKwargs = {'fooey': 'chop suey', 'success': True}

    # find and set existing, or create and set
    arg_value_dict = set_synth_arg_kwarg(my_foo,
                                    in_kwargs=newKwargs,
                                    default_value='kablooey')

    # what was returned
    print('~ multiple values returned...')
    for k, v in arg_value_dict.items():
        print("Arg '{0}':'{1}'".format(k, v))

    print('~ multiple direct property access test...')
    try:
        my_foo.fooey
        print('myFoo.{0}: {1}'.format('fooey', my_foo.fooey))
    except Exception as e:
        raise e

    try:
        my_foo.success
        print('myFoo.{0}: {1}'.format('success', my_foo.success))
    except Exception as e:
        raise e

    print('~ Starting - known failure test...')
    try:
        my_foo.knownBad
    except Exception as e:
        print(e)
        print('Test failed as expected!!!')

"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
# find_arg.py
# A simple function for arg, kwarg retrieval
# version: 0.1
# maintenance: Gallowj
# -------------------------------------------------------------------------
# -------------------------------------------------------------------------
"""Module docstring: A simple function for retrieval of arg, kwarg"""
__author__ = 'HogJonny'
# -------------------------------------------------------------------------


def find_arg(arg_pos_index=None, arg_tag=None, remove_kwarg=None,
             in_args=None, in_kwargs=None, default_value=None):
    """
    # finds and returns an arg...
    # if a positional index is given argPosIndex=0, it checks args first
    # if a argTag is given, it checks kwargs
    #    If removeKwarg=True, it will remove the found arg from kwargs
    #    * I actually want/need to do this often
    #
    # a set kwarg will ALWAYS take precident over
    # positional arg!!!
    # 
    # return outArg, args, kwargs   <-- get back modified kwargs!
    #
    # proper usage:
    #
    #    foundArg, args, kwargs = find_arg(0, 'name',)
    """

    found_arg = None

    if arg_pos_index != None:
        if not isinstance(arg_pos_index, int):
            raise TypeError('argPosIndex: accepts a index integer!\r'
                            'got: {0}'.format(arg_pos_index))

        # positional args ... check the position
        if len(in_args) > 0:
            try:
                found_arg = in_args[arg_pos_index]
            except:
                pass

    # check kwargs ... a set kwarg will ALWAYS take precident over
    # positional arg!!!
    if in_kwargs:
        found_arg = in_kwargs.get(arg_tag, default_value)  # defaults to None

    if remove_kwarg:
        if in_kwargs:
            if arg_tag in in_kwargs:
                del in_kwargs[arg_tag]

    # if we didn't find the arg/kwarg, the defualt return will be None
    return found_arg, in_kwargs
# -------------------------------------------------------------------------


###########################################################################
# --call block-------------------------------------------------------------
if __name__ == "__main__":
    print ("# ----------------------------------------------------------------------- #\r")
    print ('~ find_arg.py ... Running script as __main__')
    print ("# ----------------------------------------------------------------------- #\r")

    _DCCSI_GDEBUG = True

    from test_foo import Foo

    #######################################################################
    # Node Class
    # ---------------------------------------------------------------------
    class TestNode(Foo):
        def __init__(self, *args, **kwargs):
            super().__init__()
            self._name, kwargs = find_arg(arg_tag='foo', remove_kwarg=True,
                                          in_args=args, in_kwargs=kwargs)
            self._name, kwargs = find_arg(arg_pos_index=0, arg_tag='name',
                                          remove_kwarg=True,
                                          in_args=args, in_kwargs=kwargs)  # <-- first positional OR kwarg
            self._parent, kwargs = find_arg(arg_pos_index=1, arg_tag='parent',
                                            remove_kwarg=True,
                                            in_args=args, in_kwargs=kwargs)  # <-- second positional OR kwarg

            self._kwargsDict = {}

            # arbitrary argument properties
            # checking **kwargs, any kwargs left
            # will be used to synthesize a property
            for key, value in kwargs.items():
                self._kwargsDict[key] = value
                # synthesize(self, '{0}'.format(key), value)  <-- I have a method,
                # which synthesizes properties... with gettr, settr, etc.
                if _DCCSI_GDEBUG:
                    print("{0}:{1}".format(key, value))

        # representation
        def __repr__(self):
            return '{0}({1})\r'.format(self.__class__.__name__, self.__dict__)
        # ---------------------------------------------------------------------

    # -------------------------------------------------------------------------
    testNode = TestNode('foo')

    testNode2 = TestNode(name='fooey', parent=testNode)

    testNode3 = TestNode('kablooey', testNode2, gomer='pile')

    print ('testNode2, name: {0}, parent: {1}'.format(testNode2._name, testNode2._parent))
    print (testNode3)

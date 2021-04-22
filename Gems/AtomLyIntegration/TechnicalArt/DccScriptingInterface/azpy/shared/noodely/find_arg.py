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


def find_arg(arg_pos_index=None, argTag=None, removeKwarg=None,
             inArgs=None, inKwargs=None, defaultValue=None):
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
    if arg_pos_index != None:
        if not isinstance(arg_pos_index, int):
            raise TypeError('argPosIndex: accepts a index integer!\r'
                            'got: {0}'.format(arg_pos_index))

        # positional args ... check the position
        if len(inArgs) > 0:
            try:
                foundArg = inArgs[arg_pos_index]
            except:
                pass

    # check kwargs ... a set kwarg will ALWAYS take precident over
    # positional arg!!!
    try:
        foundArg
    except:
        foundArg = inKwargs.get(argTag, defaultValue)  # defaults to None

    if removeKwarg:
        if argTag in inKwargs:
            del inKwargs[argTag]

    # if we didn't find the arg/kwarg, the defualt return will be None
    return foundArg, inKwargs
# -------------------------------------------------------------------------


###########################################################################
# --call block-------------------------------------------------------------
if __name__ == "__main__":
    print ("# ----------------------------------------------------------------------- #\r")
    print ('~ find_arg.py ... Running script as __main__')
    print ("# ----------------------------------------------------------------------- #\r")

    _G_DEBUG = True

    from test_foo import Foo

    #######################################################################
    # Node Class
    # ---------------------------------------------------------------------
    class TestNode(Foo):
        def __init__(self, *args, **kwargs):
            super().__init__()
            self._name, kwargs = find_arg(argTag='foo', removeKwarg=True,
                                          inArgs=args, inKwargs=kwargs)
            self._name, kwargs = find_arg(arg_pos_index=0, argTag='name',
                                          removeKwarg=True,
                                          inArgs=args, inKwargs=kwargs)  # <-- first positional OR kwarg
            self._parent, kwargs = find_arg(arg_pos_index=1, argTag='parent',
                                            removeKwarg=True,
                                            inArgs=args, inKwargs=kwargs)  # <-- second positional OR kwarg

            self._kwargsDict = {}

            # arbitrary argument properties
            # checking **kwargs, any kwargs left
            # will be used to synthesize a property
            for key, value in kwargs.items():
                self._kwargsDict[key] = value
                # synthesize(self, '{0}'.format(key), value)  <-- I have a method,
                # which synthesizes properties... with gettr, settr, etc.
                if _G_DEBUG:
                    print("{0}:{1}".format(key, value))

        # representation
        def __repr__(self):
            return '{0}({1})\r'.format(self.__class__.__name__, self.__dict__)
        # ---------------------------------------------------------------------

    # -------------------------------------------------------------------------
    testNode = TestNode('foo')

    testNode2 = TestNode(name='fooey', parent=testNode)

    testNode3 = TestNode('kablooey', testNode2, goober='dufus')

    print ('testNode2, name: {0}, parent: {1}'.format(testNode2._name, testNode2._parent))
    print (testNode3)

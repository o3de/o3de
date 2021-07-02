"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# test_foo.py
# just a dumb test object
# -------------------------------------------------------------------------
__author__ = 'HogJonny'
# -------------------------------------------------------------------------

from synth import synthesize


class Foo(object):
    """
    This is a Class, it creates a Foo object... which does nothing really
    """

    __property_tag = 'fooProperty'

    # ------------------------------------------------------------------
    def __init__(self, name='Foo', value='defaultValue', *args, **kwargs):
        '''Class __init__'''
        synthesize(self, 'name', name)
        synthesize(self, Foo.__property_tag, value)

        synthesize(self, 'test', 'testValue')

        self.testDump = object.__getattribute__(self, 'test')

        # This calls a class method
        self.methodA()

    # ------------------------------------------------------------------
    def methodA(self):
        '''Class synthesized property methods self-tests'''

        """test getters"""
        print ('{0}.fooProperty is: {1}'
               ''.format(self.getName(), self.getFooProperty()))

        """test property retreival"""
        print ('{0}.testDump is: {1}'
               ''.format(self.name, self.testDump))
# ----------------------------------------------------------------------


###########################################################################
# Main Code Block, will run the tool
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # create an object from Foo Class
    myFoo = Foo()

"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

# -------------------------------------------------------------------------
# synth.py
# Convenience module for a standardized attr interface for classes/objects.
# version: 0.1
# date: 11/14/2013
# author: jGalloway
# -------------------------------------------------------------------------
__author__ = 'HogJonny'
# -------------------------------------------------------------------------
"""
This module contains the function

.synthesize(inst, name, value, readonly=False)

It is useful in object oriented class attributes, providing a stardard
interface for creating properties of classes.

Can be called within an objects Class, or an instance of an object can be
passed into the function to have the properties added.  The property will
be added and set/get/del attr interface will be created.
"""

# --------------------------------------------------------------------------


def synthesize(inst, name, value, readonly=False):
    """
    Convenience method to create getters, setters and a property for the
    instance. Should the instance already have the getters or setters
    defined this won't add them and the property will reference the already
    defined getters and setters Should be called from within __init__.

    Creates [name], _[name], get[Name], set[Name], del[Name], and  on inst.

    :param inst: An instance of the class to add the methods to.
    :param name: Base name to build function names and storage variable.
    :param value: Initial state of the created variable.

    """
    cls = type(inst)
    storageName = '_{0}'.format(name)
    getterName = 'get{0}{1}'.format(name[0].capitalize(), name[1:])
    setterName = 'set{0}{1}'.format(name[0].capitalize(), name[1:])
    deleterName = 'del{0}{1}'.format(name[0].capitalize(), name[1:])

    setattr(inst, storageName, value)

    # We always define the getter
    def buildCustomGetter(self):
        return getattr(self, storageName)

    # Add the Getter
    if not hasattr(inst, getterName):
        setattr(cls, getterName, buildCustomGetter)

    # Handle Read Only
    if readonly:
        if not hasattr(inst, name):
            setattr(cls, name,
                    property(fget=getattr(cls, getterName, None) or buildCustomGetter,
                             fdel=getattr(cls, getterName, None)))
    else:
        # We only define the setter if we arn't read only
        def buildCustomSetter(self, state):
            setattr(self, storageName, state)
        if not hasattr(inst, setterName):
            setattr(cls, setterName, buildCustomSetter)
        member = None
        if hasattr(cls, name):
            # we need to try to update the property fget, fset, fdel
            # incase the class has defined its own custom functions
            member = getattr(cls, name)
            if not isinstance(member, property):
                raise ValueError('Member "{0}" for class "{1}" exists and is not a property.'
                                 ''.format(name, cls.__name__))

        # If the class has the property or not we still try to set it
        setattr(cls, name,
                property(fget=getattr(member, 'fget', None)
                         or getattr(cls, getterName, None)
                         or buildCustomGetter,
                         fset=getattr(member, 'fset', None)
                         or getattr(cls, setterName, None)
                         or buildCustomSetter,
                         fdel=getattr(member, 'fdel', None)
                         or getattr(cls, getterName, None)))

        return getattr(inst, name)
# --------------------------------------------------------------------------


###########################################################################
# Main Code Block, will run the tool
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Self Testing"""

    from test_foo import Foo

    # create an object from Foo Class
    myFoo = Foo()

    pass


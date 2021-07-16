"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

import pymel.core as pmc
import sys
import types


def syspath():
    print 'sys.path:'
    for p in sys.path:
        print '  ' + p


def info(obj):
    """Prints information about the object."""

    lines = ['Info for %s' % obj.name(),
             'Attributes:']
    # Get the name of all attributes
    for a in obj.listAttr():
        lines.append('  ' + a.name())
    lines.append('MEL type: %s' % obj.type())
    lines.append('MRO:')
    lines.extend(['  ' + t.__name__ for t in type(obj).__mro__])
    result = '\n'.join(lines)
    print result


def _is_pymel(obj):
    try: # (1)
        module = obj.__module__ # (2)
    except AttributeError: # (3)
        try:
            module = obj.__name__ # (4)
        except AttributeError:
            return None # (5)
    return module.startswith('pymel') # (6)


def _py_to_helpstr(obj):
    if isinstance(obj, basestring):
        return 'search.html?q=%s' % (obj.replace(' ', '+'))
    if not _is_pymel(obj):
        return None
    if isinstance(obj, types.ModuleType):
        return ('generated/%(module)s.html#module-%(module)s' %
                dict(module=obj.__name__))
    if isinstance(obj, types.MethodType):
        return ('generated/classes/%(module)s/'
                '%(module)s.%(typename)s.html'
                '#%(module)s.%(typename)s.%(methname)s' % dict(
                    module=obj.__module__,
                    typename=obj.im_class.__name__,
                    methname=obj.__name__))
    if isinstance(obj, types.FunctionType):
        return ('generated/functions/%(module)s/'
                '%(module)s.%(funcname)s.html'
                '#%(module)s.%(funcname)s' % dict(
                    module=obj.__module__,
                    funcname=obj.__name__))
    if not isinstance(obj, type):
        obj = type(obj)
    return ('generated/classes/%(module)s/'
            '%(module)s.%(typename)s.html'
            '#%(module)s.%(typename)s' % dict(
                module=obj.__module__,
                typename=obj.__name__))


def test_py_to_helpstr():
    def dotest(obj, ideal):
        result = _py_to_helpstr(obj)
        assert result == ideal, '%s != %s' % (result, ideal)
    dotest('maya rocks', 'search.html?q=maya+rocks')
    dotest(pmc.nodetypes,
           'generated/pymel.core.nodetypes.html'
           '#module-pymel.core.nodetypes')
    dotest(pmc.nodetypes.Joint,
           'generated/classes/pymel.core.nodetypes/'
           'pymel.core.nodetypes.Joint.html'
           '#pymel.core.nodetypes.Joint')
    dotest(pmc.nodetypes.Joint(),
           'generated/classes/pymel.core.nodetypes/'
           'pymel.core.nodetypes.Joint.html'
           '#pymel.core.nodetypes.Joint')
    dotest(pmc.nodetypes.Joint().getTranslation,
           'generated/classes/pymel.core.nodetypes/'
           'pymel.core.nodetypes.Joint.html'
           '#pymel.core.nodetypes.Joint.getTranslation')
    dotest(pmc.joint,
           'generated/functions/pymel.core.animation/'
           'pymel.core.animation.joint.html'
           '#pymel.core.animation.joint')
    dotest(object(), None)
    dotest(10, None)
    dotest([], None)
    dotest(sys, None)


def test_py_to_helpstrFAIL():
    assert 1 == 2, '1 != 2'


import webbrowser # (1)
HELP_ROOT_URL = ('http://help.autodesk.com/cloudhelp/2018/ENU/Maya-Tech-Docs/PyMel/')# (2)


def pmhelp(obj): # (3)
    """Gives help for a pymel or python object.

    If obj is not a PyMEL object, use Python's built-in
    `help` function.
    If obj is a string, open a web browser to a search in the
    PyMEL help for the string.
    Otherwise, open a web browser to the page for the object.
    """
    tail = _py_to_helpstr(obj)
    if tail is None:
        help(obj) # (4)
    else:
        webbrowser.open(HELP_ROOT_URL + tail) # (5)


if __name__ == '__main__':
    test_py_to_helpstr()
    print 'Tests ran successfully.'

from pysbs import context
import pysbs
import logging as _logging

_LOGGER = _logging.getLogger('azpy.dcc.substance.sbs_export')

# Creation of the context
aContext = context.Context()
# Declaration of alias 'myAlias'
aContext.getUrlAliasMgr().setAliasAbsPath(aAliasName='myAlias', aAbsPath='<myAliasAbsolutePath>')
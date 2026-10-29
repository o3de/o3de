"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#!/usr/bin/python
# -*- coding: utf-8 -*-
import re
import subprocess
import sys

def findTokenToInt(output, expression):
    castStringToInt = lambda str : int(str) if str.isdigit() else -2

    #errorCode = re.search(r"Error\s\d*\s:", output)
    errorCode = re.search(expression, output)
    if not errorCode:
        return -1
    else:
        errorCode = re.sub(r"\D","", errorCode.group())
        errorCode = castStringToInt(errorCode)
        return errorCode

def firstof(haystack, *needles):
    finds = [haystack.decode('utf-8').find(n) for n in needles]
    if all(e == -1 for e in finds):
        return -1
    return min([e for e in finds if e != -1])

def clean(str):
    return str.strip().strip('"').strip("'")

def verifyPartitionPred(operands):
    if operands[1] == '==':
        return operands[0] == operands[2]
    if operands[1] == '!=':
        return operands[0] != operands[2]

def executePredicateChecks(message):
    numchecked = 0
    lookfor = "@check predicate"
    for m in re.finditer(lookfor, message.decode('utf-8')):
        numchecked = numchecked + 1
        st = m.start() + len(lookfor)
        leftover = message[st:]
        stop = firstof(leftover, '\\n', '\n', '"@')
        wholeexpr = leftover[:stop]
        operands = wholeexpr.decode('utf-8').partition("==")
        operands = [clean(s) for s in operands]
        if not verifyPartitionPred(operands):
            print ("TEST error:", operands)
            return (False, numchecked)
    return (True, numchecked)

def launchCompiler(compilerPath, options, silent):
    '''returns a tuple (standard-output-text, process-return-code)'''
    arglist = [compilerPath]
    arglist.extend(options)
    print ("    Running: ", ' '.join(arglist))
    process = subprocess.Popen(arglist, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if not silent:
        sys.stdout.write(out.decode('utf-8'))
        sys.stderr.write(err.decode('utf-8'))
    return (out, err, process.returncode)

def parseYaml(text):
    '''designed to read-in the output of --dumpsym'''
    try:
        import yaml
        #from yaml import CLoader as Loader
    except ImportError:
        print ("no yaml module. execute `pip install pyyaml`")
        return None
    try:
        return yaml.load(text, Loader=yaml.FullLoader)
    except yaml.YAMLError as e:
        print ("Parsing YAML string failed")
        if hasattr(e, "reason"):
            print ("Reason:", e.reason)
            print ("At position: {0} (line {1}) with encoding {2}".format(e.position, len(text[0:e.position].splitlines()), e.encoding))
            print ("Invalid char code:", e.character)
            print ("culprit context:", text[e.position - 8 : e.position + 8])
        else: raise
        raise
    except AttributeError as ae:
        if "has no attribute 'FullLoader'" in str(ae):
            return yaml.load(text)
        else: raise

def buildAndGetSymbols(thefile, compilerPath, silent, extraArgs=[]):
    '''returns tuple: (symbols-in-algebraic-form, ok-bool)'''
    import clr
    stdout, stderr, code = launchCompiler(compilerPath, [thefile, "--dumpsym"] + extraArgs, silent)
    if code != 0:
        if not silent: print (clr.fg.RED + clr.style.BRIGHT + "compilation failed " + clr.style.NORMAL + thefile + clr.style.RESET_ALL)
        return (None, False)
    else:
        symbols = parseYaml(stdout)
        if symbols is None:
            print (clr.fg.RED + "Parsing result of --dumpsym failed" + clr.style.RESET_ALL)
            return (None, False)
        return (symbols, code == 0)

def buildAndGetJson(thefile, compilerPath, silent, extraArgs):
    '''returns tuple: (layouts-in-json-format, ok-bool)'''
    import clr
    import json

    stdout, stderr, code = launchCompiler(compilerPath, [thefile] + extraArgs, silent)
    if code != 0:
        if not silent: print (clr.fg.RED + clr.style.BRIGHT + "compilation failed " + clr.style.NORMAL + str(extraArgs) + " " + thefile + clr.style.RESET_ALL)
        return (None, False)
    else:
        j = json.loads(stdout)
        if j is None:
            print (clr.fg.RED + "Parsing result of " + extraArgs + " failed" + clr.style.RESET_ALL)
            return (None, False)
        return (j, code == 0)

def buildAndGet(thefile, compilerPath, silent, extraArgs):
    '''returns tuple: (stdout, ok-bool)'''
    import clr
                                                                               #force silent
    stdout, stderr, code = launchCompiler(compilerPath, [thefile] + extraArgs, True)
    if code != 0:
        if not silent: print (clr.fg.RED + "compilation of " + thefile + " failed" + clr.style.RESET_ALL)
        return (None, False)

    return (stdout, code == 0)

def buildAndGetError(thefile, compilerPath, silent, extraArgs):
    '''returns tuple: (stderr, failed-bool)'''
    import clr
                                                                               #force silent
    stdout, stderr, code = launchCompiler(compilerPath, [thefile] + extraArgs, True)
    if code == 0:
        if not silent: print (clr.fg.RED + "compilation of " + thefile + " succeeded. Was expecting failure." + clr.style.RESET_ALL)
        return (None, False)

    return (stderr, code != 0)

def dumpKeywords(compilerPath):
    '''returns tuple: (categorized-tokens-in-algebraic-form, ok-bool)'''
    import clr
    stdout, stderr, code = launchCompiler(compilerPath, ["--listpredefined"], False)
    if code != 0:
        print (clr.fg.RED + "compilation failed" + clr.style.RESET_ALL)
        return (None, False)
    else:
        tokens = parseYaml(stdout)
        if tokens is None:
            print (clr.fg.RED + "Parsing result of --listpredefined failed" + clr.style.RESET_ALL)
            return (None, False)
        return (tokens, code == 0)

def verifyAllPredicates(predicates, code, silent=True):
    import clr
    import pprint
    import inspect
    allok = True
    for i, p in enumerate(predicates):
        ok = True
        try:
            ok = p()
        except Exception as e:
            print (clr.fg.RED + "exception " + clr.style.RESET_ALL + str(e))
            ok = False
        if not ok:
            try: src=inspect.getsource(p)
            except: src="<exception>"
            print (clr.fg.RED + "FAIL (" + str(i) + "):" + clr.style.RESET_ALL + src)
        allok = allok and ok
    if not allok and not silent:
        print ("dump as parsed:")
        pp = pprint.PrettyPrinter(indent=2, width=160)
        pp.pprint(code)
    return allok

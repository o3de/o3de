#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import os
import sys
import re
from util import ly_build_error


class Params(object):
    def __init__(self):
        # Cache params
        self.__params = {}

    def get(self, param_name):
        param_value = self.__params.get(param_name)
        if param_value is not None:
            return param_value
        # Call __get_${param_name} function
        func = getattr(self, '_{}__get_{}'.format(self.__class__.__name__, param_name.lower()), None)
        if func is not None:
            param_value = func()
            # Replace all ${env} in value
            if isinstance(param_value, str):
                param_value = self.__process_string(param_name, param_value)
            elif isinstance(param_value, list):
                param_value = self.__process_list(param_name, param_value)
            elif isinstance(param_value, dict):
                param_value = self.__process_dict(param_name, param_value)
            # Cache param
            self.__params[param_name] = param_value
            return param_value
        ly_build_error('method __get_{} is not defined in class {}'.format(param_name.lower(), self.__class__.__name__))

    def set(self, param_name, param_value):
        self.__params[param_name] = param_value

    def exists(self, param_name):
        try:
            self.get(param_name)
        except LyBuildError:
            return False
        return True

    def __process_string(self, param_name, param_value):
        # Find all param with format ${param}
        params = re.findall('\${(\w+)}', param_value)
        # Avoid using the same param name in value, like 'WORKSPACE': '${WORKSPACE} some string'
        if param_name in params:
            ly_build_error('The use of same parameter name({}) in value is not allowed'.format(param_name))
        # Replace ${param} with actual value
        for param in params:
            param_value = param_value.replace('${' + param + '}', self.get(param))
        return param_value

    def __process_list(self, param_name, param_value):
        processed_list = []
        for entry in param_value:
            if isinstance(entry, str):
                entry = self.__process_string(param_name, entry)
            elif isinstance(entry, list):
                entry = self.__process_list(param_name, entry)
            elif isinstance(entry, dict):
                entry = self.__process_dict(param_name, entry)
            processed_list.append(entry)
        return processed_list

    def __process_dict(self, param_name, param_value):
        for key in param_value:
            if isinstance(param_value[key], str):
                param_value[key] = self.__process_string(param_name, param_value[key])
            elif isinstance(param_value[key], list):
                param_value[key] = self.__process_list(param_name, param_value[key])
            elif isinstance(param_value[key], dict):
                param_value[key] = self.__process_dict(param_name, param_value[key])
        return param_value
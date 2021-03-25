"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from abc import ABCMeta, abstractmethod

from model.basic_resource_attributes import BasicResourceAttributes


class ResourceAbstractModel(metaclass=ABCMeta):
    """ResourceAbstractModel declares the shared interfaces among all custom resource model"""
    @abstractmethod
    def load_resource(self, resource: BasicResourceAttributes) -> None:
        """Load individual resource which will not invoke data changed signal"""
        raise NotImplementedError

    @abstractmethod
    def reset(self) -> None:
        """Reset model resources"""
        raise NotImplementedError

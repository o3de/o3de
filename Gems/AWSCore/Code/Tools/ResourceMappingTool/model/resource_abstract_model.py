"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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

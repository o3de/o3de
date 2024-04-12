#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
"""
This file contains compatibility functions
"""
from packaging.version import Version, InvalidVersion
from packaging.specifiers import SpecifierSet
import pathlib
import logging
from o3de import manifest, utils, cmake, validation
from collections import namedtuple
from resolvelib import (
    AbstractProvider,
    BaseReporter,
    InconsistentCandidate,
    ResolutionImpossible,
    Resolver,
)

logger = logging.getLogger('o3de.compatibility')
logging.basicConfig(format=utils.LOG_FORMAT)

def get_most_compatible_project_engine_path(project_path:pathlib.Path, 
                                            project_json_data:dict = None, 
                                            user_project_json_data:dict = None, 
                                            engines_json_data:dict = None) -> pathlib.Path or None:
    """
    Returns the most compatible engine path for a project based on the project's 
    'engine' field and taking into account <project_path>/user/project.json overrides
    :param project_path: Path to the project
    :param project_json_data: Json data to use to avoid reloading project.json  
    :param user_project_json_data: Json data to use to avoid reloading <project_path>/user/project.json  
    :param engines_json_data: Json data to use for engines instead of opening each file, useful for speed 
    """
    if not project_json_data:
        project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f'Failed to load project.json data from {project_path}. '
            'Please verify the path is correct, the file exists and is formatted correctly.')
        return None
    
    # look for user project.json overrides if none are provided
    if not isinstance(user_project_json_data, dict):
        user_project_json_path = pathlib.Path(project_path) / 'user' / 'project.json'
        if user_project_json_path.is_file():
            user_project_json_data = manifest.get_json_data_file(user_project_json_path, 'project', validation.always_valid)

    # take into account any user project.json overrides 
    if user_project_json_data:
        project_json_data.update(user_project_json_data)
        user_engine_path = project_json_data.get('engine_path', '')
        if user_engine_path:
            return pathlib.Path(user_engine_path)

    project_engine = project_json_data.get('engine')
    if not project_engine:
        # The project has not been registered to an engine yet
        return None

    if not engines_json_data:
        engines_json_data = manifest.get_engines_json_data_by_path()

    most_compatible_engine_path = None
    most_compatible_engine_version = None
    for engine_path, engine_json_data in engines_json_data.items():
        engine_name = engine_json_data.get('engine_name')
        engine_version = engine_json_data.get('version')
        if not engine_version:
            # use a default version number in case version is missing or empty
            engine_version = '0.0.0'

        if has_compatible_version([project_engine], engine_name, engine_version):

            # prefer the engine this project or template resides in if it is compatible
            if pathlib.PurePath(project_path).is_relative_to(pathlib.PurePath(engine_path)):
                most_compatible_engine_path = pathlib.Path(engine_path)
                most_compatible_engine_version = Version(engine_version)
                # don't consider other engines, if a user wants to override 
                # they can set the engine_path or engine in user/project.json 
                break
            elif not most_compatible_engine_path:
                most_compatible_engine_path = pathlib.Path(engine_path)
                most_compatible_engine_version = Version(engine_version)
            elif Version(engine_version) > most_compatible_engine_version:
                most_compatible_engine_path = pathlib.Path(engine_path)
                most_compatible_engine_version = Version(engine_version)
    
    return most_compatible_engine_path


def get_project_engine_incompatible_objects(project_path:pathlib.Path, engine_path:pathlib.Path = None) -> set:
    """
    Returns any incompatible objects for this project and engine.
    :param project_path: Path to the project
    :param engine_path: Optional path to the engine. If not specified, the current engine path is used
    """
    # use the specified engine path NOT necessarily engine the project is registered to
    engine_path = engine_path or manifest.get_this_engine_path()
    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error(f'Failed to load engine.json data needed for compatibility check from {engine_path}. '
            'Please verify the path is correct, the file exists and is formatted correctly.')
        return set('engine.json (missing)')

    project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f'Failed to load project.json data needed for compatibility check from {project_path}. '
            'Please verify the path is correct, the file exists and is formatted correctly.')
        return set('project.json (missing)')

    incompatible_objects = get_incompatible_objects_for_engine(project_json_data, engine_json_data)

    # verify project -> gem -> engine compatibility
    active_gem_names = project_json_data.get('gem_names',[])
    enabled_gems_file = manifest.get_enabled_gem_cmake_file(project_path=project_path)
    if enabled_gems_file and enabled_gems_file.is_file():
        active_gem_names.extend(manifest.get_enabled_gems(enabled_gems_file))
    active_gem_names = utils.get_gem_names_set(active_gem_names)

    # it's much more efficient to get all gem data once than to query them by name one by one
    all_gems_json_data = manifest.get_gems_json_data_by_name(engine_path, project_path, include_manifest_gems=True)

    # Dependency resolution takes into account gem and engine requirements so if 
    # it succeeds, all is well
    _, errors = resolve_gem_dependencies(active_gem_names, all_gems_json_data, engine_json_data)
    if errors:
        incompatible_objects.update(errors)

    return incompatible_objects


def get_incompatible_gem_dependencies(gem_json_data:dict, all_gems_json_data:dict) -> set:
    """
    Returns incompatible gem dependencies
    :param gem_json_data: gem json data dictionary
    :param all_gems_json_data: json data of all gems to use for compatibility checks
    """
    # try to avoid gem compatibility checks which incur the cost of 
    # opening many gem.json files to get version information
    gem_dependencies = gem_json_data.get('dependencies')
    if not gem_dependencies:
        return set()

    return get_incompatible_gem_version_specifiers(gem_json_data, all_gems_json_data, checked_specifiers=set())


def get_gems_project_incompatible_objects(gem_paths:list, gem_names:list, project_path:pathlib.Path) -> set():
    """
    Returns any incompatible objects for the gem names provided and project.
    :param gem_names: names of all the gems to add or replace 
    :param gem_paths: paths of all the gems 
    :param project_path: path to the project
    """
    # early out if this project has no assigned engine
    engine_path = manifest.get_project_engine_path(project_path=project_path)
    if not engine_path:
        logger.warning(f'Project at path {project_path} is not registered to an engine and compatibility cannot be checked.')
        return set()

    project_json_data = manifest.get_project_json_data(project_path=project_path)
    if not project_json_data:
        logger.error(f'Failed to load project.json data from {project_path} needed for checking compatibility')
        return set(f'project.json (missing)') 

    engine_json_data = manifest.get_engine_json_data(engine_path=engine_path)
    if not engine_json_data:
        logger.error('Failed to load engine.json data based on the engine field in project.json or detect the engine from the current folder')
        return set(f'engine.json (missing)') 

    # include the gem_paths for the gems we are adding so their
    # 'external_subdirectories' will be considered 
    all_gems_json_data = manifest.get_gems_json_data_by_name(engine_path, project_path, 
        external_subdirectories=gem_paths, include_manifest_gems=True)

    # Verify we can resolve all dependencies after adding this new gem
    active_gem_names = engine_json_data.get('gem_names',[])
    active_gem_names.extend(project_json_data.get('gem_names',[]))
    enabled_gems_file = manifest.get_enabled_gem_cmake_file(project_path=project_path)
    if enabled_gems_file and enabled_gems_file.is_file():
        active_gem_names.extend(manifest.get_enabled_gems(enabled_gems_file))

    # convert the list into a set of strings that removes any dictionaries and optional gems
    active_gem_names = utils.get_gem_names_set(active_gem_names, include_optional=False)

    # gem_names will add new gems or replace any existing gems
    active_gem_names = utils.add_or_replace_object_names(active_gem_names, gem_names)

    # Dependency resolution takes into account gem and engine requirements so if 
    # it succeeds, all is well
    _, errors = resolve_gem_dependencies(active_gem_names, all_gems_json_data, engine_json_data)
    return errors if errors else set()


def get_gem_engine_incompatible_objects(gem_json_data:dict, engine_json_data:dict, all_gems_json_data:dict) -> set:
    """
    Returns any incompatible objects for this gem and engine.
    :param gem_json_data: gem json data dictionary
    :param engine_json_data: engine json data dictionary
    :param all_gems_json_data: json data of all gems to use for compatibility checks
    """
    incompatible_objects = get_incompatible_objects_for_engine(gem_json_data, engine_json_data)
    return incompatible_objects.union(get_incompatible_gem_dependencies(gem_json_data, all_gems_json_data))


def get_incompatible_objects_for_engine(object_json_data:dict, engine_json_data:dict) -> set:
    """
    Returns any incompatible objects for this object and engine.
    If a compatible_engine entry only has an engine name, it is assumed compatible with every engine version with that name.
    If an engine_api_version entry only has an api name, it is assumed compatible with every engine api version with that name.
    :param object_json_data: the json data for the object 
    :param engine_json_data: the json data for the engine
    """
    compatible_engines = object_json_data.get('compatible_engines')
    engine_api_version_specifiers = object_json_data.get('engine_api_dependencies')

    # early out if there are no restrictions
    if not compatible_engines and not engine_api_version_specifiers:
        return set() 

    engine_name = engine_json_data['engine_name']
    engine_version = engine_json_data.get('version')

    incompatible_objects = set()
    if compatible_engines:
        if engine_version and has_compatible_version(compatible_engines, engine_name, engine_version):
            return set() 
        elif not engine_version and has_compatible_name(compatible_engines, engine_name):
            # assume an engine with no version is compatible
            return set()

        # object is not known to be compatible with this engine
        incompatible_objects.add(f"{engine_name} {engine_version} does not match any version specifiers in the list of compatible engines: {compatible_engines}")

    if engine_api_version_specifiers:
        engine_api_versions = engine_json_data.get('api_versions')
        if not engine_api_versions:
            # assume not compatible if no engine api version information is available
            return incompatible_objects.union(set(engine_api_version_specifiers))

        incompatible_apis = set()
        for api_version_specifier in engine_api_version_specifiers:
            api_name, _ = utils.get_object_name_and_optional_version_specifier(api_version_specifier)
            engine_api_version = engine_api_versions.get(api_name,'')
            if not has_compatible_version([api_version_specifier], api_name, engine_api_version):
                if engine_api_version:
                    incompatible_apis.add(f"The engine '{api_name}' version '{engine_api_version}' doesn't satisfy requirement '{api_version_specifier}'")
                else:
                    incompatible_apis.add(f"The engine doesn't satisfy the requirement '{api_version_specifier}'")
        
        # if an object is compatible with all APIs then it's compatible even
        # if that engine is not listed in the `compatible_engines` field
        if not incompatible_apis:
            return set()
        else:
            incompatible_objects.update(incompatible_apis)

    return incompatible_objects


def get_incompatible_gem_version_specifiers(gem_json_data:dict, all_gems_json_data:dict, checked_specifiers:set) -> set:
    """
    Returns a set of gem version specifiers that are not compatible with the provided gem's dependencies
    If a dependency entry only has a gem name, it is assumed compatible with every gem version with that name.
    :param gem_json_data: gem json data with optional 'dependencies' list
    :param all_gems_json_data: json data of all gems to use for compatibility checks
    :param checked_specifiers: a set of all gem specifiers already checked to avoid cycles
    """
    gem_version_specifier_list = gem_json_data.get('dependencies')
    if not gem_version_specifier_list:
        return set()

    if not all_gems_json_data:
        # no gems are available
        return set(gem_version_specifier_list)

    incompatible_gem_version_specifiers = set()

    for gem_version_specifier in gem_version_specifier_list:
        if gem_version_specifier in checked_specifiers:
            continue

        checked_specifiers.add(gem_version_specifier)

        gem_name, version_specifier = utils.get_object_name_and_optional_version_specifier(gem_version_specifier)
        if not gem_name in all_gems_json_data:
            incompatible_gem_version_specifiers.add(f"{gem_json_data['gem_name']} is missing the dependency {gem_version_specifier}")
            continue

        all_candidate_incompatible_gem_version_specifiers = set() 
        # check every version of this gem and only record incompatible specifiers
        # when we don't find one that is compatible
        found_candidate = False
        for candidate_gem_json_data in all_gems_json_data[gem_name]:
            if version_specifier:
                gem_version = candidate_gem_json_data.get('version')
                if gem_version and not has_compatible_version([gem_version_specifier], gem_name, gem_version):
                    if not found_candidate:
                        all_candidate_incompatible_gem_version_specifiers.add(f"{gem_json_data['gem_name']} depends on {gem_version_specifier} but {gem_name} version {gem_version} was found")
                    continue
                else:
                    found_candidate = True
            else:
                found_candidate = True

            if found_candidate:
                # clear all previous incompatible errors because we found a candidate
                # we only want to show dependency incompatibility 
                all_candidate_incompatible_gem_version_specifiers = set()

            # check dependencies recursively 
            candidate_incompatible_gem_version_specifiers = get_incompatible_gem_version_specifiers(candidate_gem_json_data, all_gems_json_data, checked_specifiers)
            if candidate_incompatible_gem_version_specifiers:
                all_candidate_incompatible_gem_version_specifiers.update(candidate_incompatible_gem_version_specifiers)
            else:
                all_candidate_incompatible_gem_version_specifiers = set()
                break
        
        incompatible_gem_version_specifiers.update(all_candidate_incompatible_gem_version_specifiers)

    return incompatible_gem_version_specifiers


def has_compatible_name(name_and_version_specifier_list:list, object_name:str) -> bool:
    """
    Returns True if the object_name matches an entry in the name_and_version_specifier_list
    :param name_and_version_specifier_list: a list of names and (optional)version specifiers
    :param object_name: the object name
    """
    for name_and_version_specifier in name_and_version_specifier_list:
        # only accept a name without a version specifier
        if object_name == name_and_version_specifier:
            return True

    return False 


def has_compatible_version(name_and_version_specifier_list:list, object_name:str, object_version:str) -> bool:
    """
    returns True if the object_name matches an entry in the name_and_version_specifier_list that has no version,
    or if the object_name matches and the object_version is compatible with the version specifier.
    :param name_and_version_specifier_list: a list of names and (optional)version specifiers
    :param object_name: the object name
    :param object_version: the object version
    """
    try:
        version = Version(object_version)
    except InvalidVersion as e:
        logger.warning(f'Failed to parse version specifier {object_version}, please verify it is PEP 440 compatible.')
        return False

    for name_and_version_specifier in name_and_version_specifier_list:
        # it's possible we received a name without a version specifier
        if object_name == name_and_version_specifier:
            return True

        try:
            name, version_specifier = utils.get_object_name_and_version_specifier(name_and_version_specifier)
            if name == object_name and version in SpecifierSet(version_specifier):
                return True
        except (utils.InvalidObjectNameException, utils.InvalidVersionSpecifierException):
            # skip invalid specifiers
            pass

    return False 

class GemRequirement(namedtuple("GemRequirement", ["name", "specifier"])):
    def __repr__(self):
        return f'<GemRequirement({self.name}{self.specifier if self.specifier else ""})>'

    def identify(self):
        # IMPORTANT don't use the specifier or we will get multiple mappings
        # for gems instead of unique mappings for each gem name
        return f"GemRequirement:{self.name}"

    def failure_reason(self, object_name):
        return f'{object_name} requires {self.name}{self.specifier if self.specifier else ""}'

class EngineRequirement(namedtuple("EngineRequirement", ["gem_json_data"])):
    def __repr__(self):
        return f"<EngineRequirement({self.gem_json_data.get('compatible_engines','')}{self.gem_json_data.get('engine_api_dependencies','')})>"

    def identify(self):
        # Use a singular identifier because there is only one engine being considered
        return "EngineRequirement"

    def failure_reason(self, engine_json_data):
        gem_name = self.gem_json_data['gem_name']
        incompatible_engine_objects = get_incompatible_objects_for_engine(self.gem_json_data, engine_json_data)
        incompatible_engine_objects = [f'{gem_name} is incompatible because: {error}' for error in incompatible_engine_objects]
        return f'\n'.join(incompatible_engine_objects)

class EngineCandidate(namedtuple("Candidate",["name", "version","requirements", "engine_json_data"])):
    def __repr__(self):
        return f"<Engine {self.name}=={self.version}>"

    def identify(self):
        return repr(self)

    def __hash__(self) -> int:
        return hash(('engine', self.name, self.version))

class GemCandidate(namedtuple("Candidate",["name", "version","requirements","gem_json_data"])):
    def __repr__(self):
        return f"<Gem {self.name}=={self.version}>"

    def identify(self):
        # IMPORTANT identify with just the name and don't include a specifier
        # or we will get conflicting mappings. Use a prefix to avoid collisions with Engines
        #return f"Gem:{self.name}"
        return repr(self)

    def __hash__(self) -> int:
        return hash(('gem', self.name, self.version))

class GemDependencyProvider(AbstractProvider):
    def __init__(self, candidates):
        self.candidates = candidates

    def identify(self, requirement_or_candidate):
        return requirement_or_candidate.identify()

    def get_preference(self, identifier, resolutions, candidates, information, **_):
        # This function could be used in the future to make good choices 
        # to speed up dependency resolution when selecting the order in which
        # candidates are evaluated
        return 0

    def find_matches(self, identifier, requirements, incompatibilities):
        # Returns a list of candidates that satisfy the requirements
        # sorted by version so we prefer higher version numbers
        name = identifier
        return sorted(
            (c
            for c in self.candidates
            if all(self.is_satisfied_by(r, c) for r in requirements[name])
            and all(c.version != i.version for i in incompatibilities[name])),
            key=lambda candidate: candidate.version,
            reverse=True
        )

    def is_satisfied_by(self, requirement, candidate):
        if isinstance(candidate, EngineCandidate) and isinstance(requirement, EngineRequirement):
            incompatible_engine_objects = get_incompatible_objects_for_engine(requirement.gem_json_data, candidate.engine_json_data)
            # If the gem has engine dependencies and we fail to satisfy them
            # incompatible_engine_objects will contain the set of every failed
            # dependency.  We could store these on the EngineRequirement, but 
            # won't need it if a valid candidate is found.
            return not incompatible_engine_objects
        elif isinstance(candidate, GemCandidate) and isinstance(requirement,GemRequirement):
            return (
                candidate.name == requirement.name
                # It's much faster to check if specifier is None
                # than to check if candidate.version in ">=0.0.0"
                and (requirement.specifier is None or candidate.version in requirement.specifier)
            )
        else:
            # GemCandidates do no satisfy EngineRequirements
            # EngineCandidates do not satisfy GemRequirements
            return False

    def get_dependencies(self, candidate):
        return candidate.requirements

def resolve_gem_dependencies(gem_names:list, all_gem_json_data:dict, engine_json_data:dict, include_optional=False) -> tuple:
    # Start with the engine candidate using a version of 0.0.0 for any
    # engine that has no version field (older engine)
    candidates = [
        EngineCandidate(engine_json_data.get('engine_name'), engine_json_data.get('version','0.0.0'), [], engine_json_data)
    ]

    provided_unique_service_gem_map = {}

    # Add all gem candidates and their requirements
    for gem_name, gem_versions_json_data in all_gem_json_data.items():
        for gem_json_data in gem_versions_json_data:

            requirements = []

            # If the version field exists but is empty use '0.0.0'
            # This gives us a preference for gems with version fields
            gem_version = gem_json_data.get('version','0.0.0') or '0.0.0'
            if gem_version[:1] == '$':
                # Special case where this version is a variable in a template
                # and looks like '${Version}' or similar
                gem_version = '0.0.0'
            gem_dependencies = gem_json_data.get('dependencies',[])

            # Add gem requirements
            gem_dependency_names = utils.get_gem_names_set(gem_dependencies, include_optional=include_optional)
            for gem_dependency in gem_dependency_names:
                dep_name, dep_version_specifier = utils.get_object_name_and_optional_version_specifier(gem_dependency)
                if dep_version_specifier:
                    dep_version_specifier = SpecifierSet(dep_version_specifier)

                requirements.append(GemRequirement(dep_name, dep_version_specifier))
            
            # Add engine requirements. Technically "compatible_engines" should not
            # be used for incompatibility, and this should be addressed in the future.
            compatible_engines = gem_json_data.get('compatible_engines',[])
            engine_api_dependencies = gem_json_data.get('engine_api_dependencies',[])
            if compatible_engines or engine_api_dependencies:
                requirements.append(EngineRequirement(gem_json_data))

            candidate = GemCandidate(gem_name, Version(gem_version), requirements, gem_json_data)
            candidates.append(candidate)

    provider = GemDependencyProvider(candidates)
    reporter = BaseReporter()
    resolver = Resolver(provider=provider, reporter=reporter)

    # Add all project gem dependency requirements
    project_gem_requirements = set()
    for gem_name in gem_names:
        dep_name, dep_version_specifier = utils.get_object_name_and_optional_version_specifier(gem_name)
        if dep_version_specifier:
            dep_version_specifier = SpecifierSet(dep_version_specifier)
        project_gem_requirements.add(GemRequirement(dep_name, dep_version_specifier))

        # Track all the identified unique service providers and the gems that provide it
        gem_spec_list = all_gem_json_data.get(dep_name, [])
        for gem_spec in gem_spec_list:
            provided_unique_service = gem_spec.get('provided_unique_service', None)
            if provided_unique_service:
                provided_unique_service_gem_map.setdefault(provided_unique_service, []).append(gem_spec.get('display_name',gem_name))

    result_mapping = None
    errors = set()

    # the resolver uses a single "round" to try and pin a single dependency
    # so we need at least enough rounds as we have gems in the dependency tree
    # for comparison, pypip uses 2 million rounds
    # https://github.com/pypa/pip/blob/main/src/pip/_internal/resolution/resolvelib/resolver.py#L91
    num_gems = sum(len(gem_versions) for _,gem_versions in all_gem_json_data.items()) 
    try_to_avoid_resolution_too_deep = max(2000000, num_gems)
    try:
        result = resolver.resolve(requirements=project_gem_requirements, max_rounds=try_to_avoid_resolution_too_deep)

        # Remove any EngineCandidates that may appear in the mappings
        result_mapping = {k: v for k, v in result.mapping.items() if isinstance(v, GemCandidate)}
    except InconsistentCandidate as e:
        # An error exists in our dependency resolver provider
        # need to fix find_matches() and/or is_satisfied_by().
        errors.add(f'An error exists in the dependency resolver provider resulting in an inconsistent candidate {e.candidate} with criterion {e.criterion}')
    except ResolutionImpossible as e:
        reason = 'The following dependency requirements could not be satisfied:'
        for cause in e.causes:
            reason += '\n'
            if isinstance(cause.requirement, EngineRequirement):
                reason += cause.requirement.failure_reason(engine_json_data)
            else:
                # If cause.parent isn't set it's a project gem dependency
                reason += cause.requirement.failure_reason(cause.parent if cause.parent else 'The project')

        errors.add(reason)

    # Finally, check if there are multiple gems enabled that provide duplicate unique services
    for unique_service, gem_provider_list in provided_unique_service_gem_map.items():
        if len(gem_provider_list) > 1:
            errors.add(f"Multiple gems enabled ({', '.join(gem_provider_list)}) that provide the "
                       f"same unique service '{unique_service}' detected. This will cause failures "
                       f"during Gem loading and initialization.")

    return result_mapping, errors

import pytest

# Editor test files collection
def pytest_pycollect_makeitem(collector, name, obj):
    import inspect
    
    #print(obj)
    if inspect.isclass(obj):
        for base in obj.__bases__:
            if hasattr(base, "pytest_custom_makeitem"):
                return base.pytest_custom_makeitem(collector, name, obj)
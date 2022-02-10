
# E:\Depot\o3de_dccsi\Gems\Atom\Feature\Common\Assets\Materials\Types
# Add jsonschema to requirements.txt


import json
from jsonschema import validate

class TemplateGenerator:
    def __init__(self, template_type, output_path):

        self.template_type = template_type
        self.output_path = output_path


    def validate_json(self, target_json):
        prin




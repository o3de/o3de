import config
from dynaconf import settings
from pathlib import Path
import markdown
import logging
import re


_LOGGER = logging.getLogger('azpy.o3de.utils.markdown_conversion')
_LOGGER.info('Markdown Conversion module added.')


def convert_markdown_file(markdown_file):
    with open(markdown_file, 'r') as file:
        formatted_text = markdown.markdown(file.read())
        md_location = Path(markdown_file).parent.as_posix()
        formatted_text = modify_image_paths(md_location, formatted_text)
        return formatted_text


def modify_image_paths(md_location, html):
    image_urls = re.findall(r'<img[^<>]+src=["\']([^"\'<>]+\.(?:gif|png|jpe?g))["\']', html, re.I)
    updated_html = html
    for found_path in image_urls:
        modified_path = Path(f'{md_location}') / found_path
        if modified_path.exists():
            updated_html = re.sub(found_path, modified_path.as_posix(), updated_html)
    return updated_html


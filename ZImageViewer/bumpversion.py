import re
import codecs

# global variables
major = 0
minor = 0
build = 0
revision = 0

def open_encoded_file(rc_file_path):
    with open(rc_file_path, 'rb') as file:
        # Read the first few bytes to detect the BOM
        bom = file.read(4)
        file.seek(0)  # Reset the file pointer

        if bom.startswith(codecs.BOM_UTF16_LE):
            encoding = 'utf-16'
        else:
            encoding = 'utf-8'  # Default to UTF-8 if no BOM is detected

        with open(rc_file_path, 'r', encoding=encoding) as rc_file:
            rc_content = rc_file.read()

    return rc_content

# Function to increment the version number
def bump_version(rc_content, increment_type='major'):
    # Define regular expressions to match the version number
    version_regex = r'FILEVERSION (\d+),(\d+),(\d+),(\d+)'
    product_version_regex = r'PRODUCTVERSION (\d+),(\d+),(\d+),(\d+)'

    value_version_regex = r'VALUE "FileVersion", "(\d+).(\d+).(\d+).(\d+)"'
    value_product_regex = r'VALUE "ProductVersion", "(\d+).(\d+).(\d+).(\d+)"'



    # Find and extract the current version numbers
    file_version_match = re.search(version_regex, rc_content)
    product_version_match = re.search(product_version_regex, rc_content)
    print(file_version_match)
    print(product_version_match)


    if file_version_match and product_version_match:
        global major, minor, build, revision
        major, minor, build, revision = map(int, file_version_match.groups())

        if increment_type == 'major':
            major += 1
        elif increment_type == 'minor':
            minor += 1
        elif increment_type == 'build':
            build += 1
        elif increment_type == 'revision':
            revision += 1

        # Replace the old version numbers with the incremented values
        rc_content = re.sub(version_regex, f'FILEVERSION {major},{minor},{build},{revision}', rc_content)
        rc_content = re.sub(product_version_regex, f'PRODUCTVERSION {major},{minor},{build},{revision}', rc_content)
        rc_content = re.sub(value_version_regex, f'VALUE "FileVersion", "{major}.{minor}.{build}.{revision}"', rc_content)
        rc_content = re.sub(value_product_regex, f'VALUE "ProductVersion", "{major}.{minor}.{build}.{revision}"', rc_content)

        return rc_content
    else:
        print("Version numbers not found in the .rc file.")
        return None

# Example usage
if __name__ == "__main__":
    rc_file_path = 'ZImageViewer.rc'
    increment_type = 'revision'  # Change this to 'minor', 'build', or 'revision' as needed

    rc_content = open_encoded_file(rc_file_path)
    print(rc_content)

    updated_rc_content = bump_version(rc_content, increment_type)

    if updated_rc_content:
        with open(rc_file_path, 'w') as file:
            file.write(updated_rc_content)
        print(f"Version number bumped {increment_type} and saved to {rc_file_path}")

    cfg_file_path = 'install/install.cfg'
    cfg_content = open_encoded_file(cfg_file_path)
    
    print("--printing cfg_content--")
    print(cfg_content)
    
    cfg_version_regex = r'appver=(\d+).(\d+).(\d+).(\d+)'
    cfg_content = re.sub(cfg_version_regex, f'appver={major}.{minor}.{build}.{revision}', cfg_content)
    with open(cfg_file_path, 'w') as file:
        file.write(cfg_content)
    print(f"cfg version updated")
    
    print("--writing release index.html--")
    indexhtml_template_file_path = 'index.html.template.html' 
    indexhtml_final_file_path = 'index.html'
    indexhtml_content = open_encoded_file(indexhtml_template_file_path)
    indexhtml_version_regex = r'Version=(\d+).(\d+).(\d+).(\d+)'
    indexhtml_content = re.sub(indexhtml_version_regex, f'Version=<b>{major}.{minor}.{build}.{revision}</b>', indexhtml_content)
    
    with open(indexhtml_final_file_path, 'w') as file:
        file.write(indexhtml_content)
    
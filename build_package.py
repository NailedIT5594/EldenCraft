import os
import shutil
import zipfile

def create_release_zip(output_zip, files_to_include):
    with zipfile.ZipFile(output_zip, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for file in files_to_include:
            if os.path.isfile(file):
                zipf.write(file, os.path.basename(file))
            elif os.path.isdir(file):
                for root, _, files in os.walk(file):
                    for f in files:
                        zipf.write(os.path.join(root, f), os.path.relpath(os.path.join(root, f), start=os.path.join(file, '..')))

def main():
    output_zip = 'release.zip'
    files_to_include = [
        'weapons_sheet.json',
        'movement_sheet.json',
        'compiled_output',  # Replace with the actual path to your compiled output or mod folders
        'mod_folder'        # Replace with the actual path to your mod folders
    ]

    create_release_zip(output_zip, files_to_include)
    print(f"Release package created: {output_zip}")

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
import os
import sys
import re
import json
from pathlib import Path
from collections import defaultdict

RESOURCE_EXTENSIONS = {
    'image': {'.png', '.jpg', '.jpeg', '.bmp', '.gif', '.tiff', '.webp', '.icns'},
    'audio': {'.wav', '.mp3', '.ogg', '.flac', '.aac', '.mp4', '.avi'},
    'font': {'.ttf', '.otf'},
    'font_data': {'.fnt'},
    'spritesheet': {'.json'},
    'gui': {'.rml', '.rcss', '.lua'},
    'data': {'.json', '.xml', '.txt', '.csv'},
}

RESOURCE_TYPE_HINTS = {
    'Resources/examples/app': {'image', 'spritesheet', 'gui', 'data'},
    'Resources/examples/audio': {'audio'},
    'Resources/tests/actions/ActionTest': {'image', 'spritesheet', 'font_data', 'font'},
    'Resources/tests/audio/ALAudioTest': {'audio'},
    'Resources/tests/graphics/BMFontTest': {'font_data', 'image', 'font'},
    'Resources/tests/graphics/BitmapFontTest': {'image'},
    'Resources/tests/graphics/GradientTest': {'image', 'spritesheet', 'data'},
    'Resources/tests/graphics/SpriteTest': {'image'},
    'Resources/tests/graphics/TrueTypeFontTest': {'font'},
    'Resources/tests/gui': {'gui', 'font', 'image', 'spritesheet', 'data'},
    'Resources/tests/serializers/json': {'data', 'spritesheet'},
    'Resources/tests/serializers/xml': {'data', 'spritesheet'},
    'Resources/SharedSupport/InputDevices': {'data'},
    'Resources/SharedSupport/TexturePacker/nomlib': {'spritesheet'},
}

MANIFEST_EXCLUDE_NAMES = {
    'spritesheet.json',
    'resource-cfg.json',
    'cursors.json',
    'menu_elements.json',
    'faces.json',
    'auctions.json',
    'inventory.json',
    'omnom.json',
    'gameover.json',
}

OUTPUT_FILENAME_PATTERNS = {
    'screenshot.png',
}

OUTPUT_FILENAME_PREFIXES = (
    'output_',
    'config_new',
    'config2',
    'config3',
    'FileOutputStream',
)

FAKE_TEST_FILENAMES = {
    'FakeImageFile1_0.png',
    'FakeImageFile2_0.png',
    'FakeImageFile3_0.png',
    'FakeImageFile4_0.png',
    'Image0.png',
    'Image1.png',
    'Image2.png',
    'Image3.png',
    'info.json',
    'IX.png',
}

TEST_STRING_ONLY_FILENAMES = {
    'times.ttf',
    'VIII.png',
    'images/board.png',
    'images/game-over_background.png',
    'cards.json',
    'ah.xml',
    'inv.xml',
}

HARDCODED_PATH_IGNORE = {
    'Resources/audio/hello.wav',
    'Resources/gui',
    'Resources/json/config.json',
}

ERROR_MESSAGE_PREFIXES = (
    'Could not load',
    'Failed to load',
    'Unable to load',
    'Error loading',
)

class ResourceChecker:
    def __init__(self, project_root, strict=False):
        self.project_root = Path(project_root).resolve()
        self.strict = strict
        self.errors = []
        self.warnings = []
        self.manifest_paths = {}
        self._seen_errors = set()
        self._seen_warnings = set()
        self.resources_dir = self.project_root / 'Resources'
        self.examples_dir = self.project_root / 'examples'
        self.tests_src_dir = self.project_root / 'tests' / 'src'
        self.tests_include_dir = self.project_root / 'tests' / 'include'

    def error(self, msg):
        if msg not in self._seen_errors:
            self._seen_errors.add(msg)
            self.errors.append(f'ERROR: {msg}')

    def warning(self, msg, strict_error=False):
        if self.strict and strict_error:
            self.error(msg)
            return
        if msg not in self._seen_warnings:
            self._seen_warnings.add(msg)
            self.warnings.append(f'WARNING: {msg}')

    def classify_extension(self, filename):
        ext = Path(filename).suffix.lower()
        for type_name, exts in RESOURCE_EXTENSIONS.items():
            if ext in exts:
                return type_name
        return None

    def strip_cpp_comments(self, content):
        result = []
        in_block_comment = False
        i = 0
        lines = content.split('\n')
        for line in lines:
            stripped = ''
            in_string = False
            j = 0
            while j < len(line):
                ch = line[j]
                if not in_block_comment:
                    if ch == '"' and (j == 0 or line[j-1] != '\\'):
                        in_string = not in_string
                        stripped += ch
                    elif not in_string and ch == '/' and j + 1 < len(line) and line[j+1] == '/':
                        break
                    elif not in_string and ch == '/' and j + 1 < len(line) and line[j+1] == '*':
                        in_block_comment = True
                        j += 1
                    else:
                        stripped += ch
                else:
                    if ch == '*' and j + 1 < len(line) and line[j+1] == '/':
                        in_block_comment = False
                        j += 1
                j += 1
            result.append(stripped)
        return '\n'.join(result)

    def find_manifest_files(self):
        manifests = []
        manifest_patterns = [
            self.resources_dir / 'examples' / '*.json',
            self.resources_dir / 'tests' / '*.json',
            self.resources_dir / 'tests' / 'actions' / '*.json',
            self.resources_dir / 'tests' / 'audio' / '*.json',
            self.resources_dir / 'tests' / 'graphics' / '*.json',
            self.resources_dir / 'tests' / 'serializers' / '*.json',
            self.resources_dir / 'SharedSupport' / '*.json',
        ]
        for pattern in manifest_patterns:
            for f in self.project_root.glob(str(pattern.relative_to(self.project_root))):
                if f.name in MANIFEST_EXCLUDE_NAMES:
                    continue
                manifests.append(f)
        return sorted(manifests)

    def parse_manifest(self, manifest_path):
        try:
            with open(manifest_path, 'r', encoding='utf-8') as f:
                content = f.read()
            stripped = self.strip_cpp_comments(content)
            data = json.loads(stripped)
        except json.JSONDecodeError as e:
            self.error(f'Invalid JSON in manifest {manifest_path.relative_to(self.project_root)}: {e}')
            return {}
        except Exception as e:
            self.error(f'Cannot read manifest {manifest_path.relative_to(self.project_root)}: {e}')
            return {}
        return data

    def check_manifest(self, manifest_path, manifest_data):
        rel_manifest = manifest_path.relative_to(self.project_root)
        for node_name, node_data in manifest_data.items():
            if not isinstance(node_data, dict):
                continue
            if 'path' not in node_data:
                continue
            res_path = node_data.get('path', '')
            search_prefixes = node_data.get('search_prefix', [])
            if not res_path:
                continue
            full_path = self.project_root / res_path
            if not full_path.exists():
                self.error(
                    f'Manifest {rel_manifest} node [{node_name}]: '
                    f'resource path does not exist: {res_path}'
                )
                continue
            if not full_path.is_dir():
                self.error(
                    f'Manifest {rel_manifest} node [{node_name}]: '
                    f'resource path is not a directory: {res_path}'
                )
                continue
            if not search_prefixes:
                self.warning(
                    f'Manifest {rel_manifest} node [{node_name}]: '
                    f'no search_prefix defined'
                )
            key = f'{rel_manifest}:{node_name}'
            self.manifest_paths[key] = {
                'manifest': manifest_path,
                'node': node_name,
                'path': res_path,
                'full_path': full_path,
                'search_prefixes': search_prefixes,
            }

    def collect_actual_files(self):
        actual_files = defaultdict(list)
        resource_dirs = [
            self.resources_dir / 'examples',
            self.resources_dir / 'tests',
            self.resources_dir / 'SharedSupport' / 'InputDevices',
            self.resources_dir / 'SharedSupport' / 'TexturePacker',
        ]
        for res_dir in resource_dirs:
            if not res_dir.exists():
                continue
            for root, dirs, files in os.walk(res_dir):
                for f in files:
                    if f.startswith('.'):
                        continue
                    filepath = Path(root) / f
                    rel_path = filepath.relative_to(self.project_root)
                    actual_files[str(rel_path.parent)].append(f)
        return actual_files

    def is_error_message_string(self, match_text, full_content, start_pos):
        line_start = full_content.rfind('\n', 0, start_pos) + 1
        line_end = full_content.find('\n', start_pos)
        if line_end == -1:
            line_end = len(full_content)
        line = full_content[line_start:line_end]
        for prefix in ERROR_MESSAGE_PREFIXES:
            if prefix.lower() in line.lower():
                return True
        return False

    def scan_source_for_resources(self):
        referenced_resources = []
        source_files = []
        for ext in ('*.cpp', '*.hpp', '*.h'):
            source_files.extend(self.examples_dir.rglob(ext))
            source_files.extend(self.tests_src_dir.rglob(ext))
            source_files.extend(self.tests_include_dir.rglob(ext))
        resource_exts = r'png|wav|jpg|jpeg|bmp|gif|tiff|webp|ttf|otf|fnt|json|rml|rcss|lua|xml|txt|mp3|ogg|flac|aac|mp4|avi|icns'
        resource_string_pattern = re.compile(
            r'"([^"]+\.(?:' + resource_exts + r'))"'
        )
        path_concat_pattern = re.compile(
            r'(\w+)\.path\(\)\s*\+\s*"([^"]+)"'
        )
        hardcoded_path_pattern = re.compile(
            r'"(Resources/[^"]+|examples/Resources/[^"]+)"'
        )
        load_file_pattern = re.compile(
            r'load_file\s*\(\s*"?([^",)]+)"?\s*,'
        )
        manifest_load_pattern = re.compile(
            r'load_file\s*\(\s*(\w+)\s*,\s*"([^"]+)"'
        )
        res_var_pattern = re.compile(
            r'(?:const\s+)?(?:std::string|auto)\s+(RESOURCE_\w+|RES_\w+|SHEET_FILE_PATH|TEX_FILE_PATH|TEXTURE_FILENAME|SPRITE_SHEET_FILENAME|sheet_coords|sheet_img|RES_FILENAME|RES_FILE)\s*=\s*"([^"]+)"'
        )
        for src in sorted(source_files):
            try:
                with open(src, 'r', encoding='utf-8') as f:
                    content = f.read()
            except Exception:
                continue
            rel_src = src.relative_to(self.project_root)
            clean_content = self.strip_cpp_comments(content)
            for m in res_var_pattern.finditer(clean_content):
                var_name, filename = m.group(1), m.group(2)
                if filename in OUTPUT_FILENAME_PATTERNS or filename in FAKE_TEST_FILENAMES or filename in TEST_STRING_ONLY_FILENAMES:
                    continue
                if any(filename.startswith(p) for p in OUTPUT_FILENAME_PREFIXES):
                    continue
                referenced_resources.append({
                    'source': rel_src,
                    'line': clean_content[:m.start()].count('\n') + 1,
                    'name': var_name,
                    'filename': filename,
                    'kind': 'const_var',
                })
            for m in path_concat_pattern.finditer(clean_content):
                filename = m.group(2)
                line_no = clean_content[:m.start()].count('\n') + 1
                in_examples = str(rel_src).startswith('examples/')
                if in_examples or self.strict:
                    self.error(
                        f'{rel_src}:{line_no}: '
                        f'Manual path concatenation detected '
                        f'({m.group(1)}.path() + "{filename}"). '
                        f'Use SearchPath::load_file() instead.'
                    )
                if filename in OUTPUT_FILENAME_PATTERNS or filename in FAKE_TEST_FILENAMES or filename in TEST_STRING_ONLY_FILENAMES:
                    continue
                if any(filename.startswith(p) for p in OUTPUT_FILENAME_PREFIXES):
                    continue
                referenced_resources.append({
                    'source': rel_src,
                    'line': line_no,
                    'name': f'{m.group(1)}.path()+',
                    'filename': filename,
                    'kind': 'path_concat',
                })
            for m in load_file_pattern.finditer(clean_content):
                filename = m.group(1)
                if not filename.endswith(('.json', '.xml', '.txt')):
                    continue
                if filename in OUTPUT_FILENAME_PATTERNS or filename in FAKE_TEST_FILENAMES or filename in TEST_STRING_ONLY_FILENAMES:
                    continue
                if any(filename.startswith(p) for p in OUTPUT_FILENAME_PREFIXES):
                    continue
                referenced_resources.append({
                    'source': rel_src,
                    'line': clean_content[:m.start()].count('\n') + 1,
                    'name': 'load_file',
                    'filename': filename,
                    'kind': 'load_file',
                })
            for m in resource_string_pattern.finditer(clean_content):
                filename = m.group(1)
                if filename in OUTPUT_FILENAME_PATTERNS or filename in FAKE_TEST_FILENAMES or filename in TEST_STRING_ONLY_FILENAMES:
                    continue
                if any(filename.startswith(p) for p in OUTPUT_FILENAME_PREFIXES):
                    continue
                if filename in HARDCODED_PATH_IGNORE:
                    continue
                if filename.startswith('Resources/') or filename.startswith('examples/'):
                    continue
                if self.is_error_message_string(filename, clean_content, m.start()):
                    continue
                already_found = any(
                    r['source'] == rel_src and r['filename'] == filename
                    for r in referenced_resources
                )
                if already_found:
                    continue
                referenced_resources.append({
                    'source': rel_src,
                    'line': clean_content[:m.start()].count('\n') + 1,
                    'name': 'string_literal',
                    'filename': filename,
                    'kind': 'string_literal',
                })
            seen_hardcoded = set()
            in_examples = str(rel_src).startswith('examples/')
            for m in hardcoded_path_pattern.finditer(clean_content):
                path_val = m.group(1)
                line_no = clean_content[:m.start()].count('\n') + 1
                key = (str(rel_src), line_no, path_val)
                if key in seen_hardcoded:
                    continue
                seen_hardcoded.add(key)
                if path_val in HARDCODED_PATH_IGNORE and not (in_examples or self.strict):
                    continue
                self.error(
                    f'{rel_src}:{line_no}: '
                    f'Hardcoded resource path detected (use SearchPath instead): '
                    f'{path_val}'
                )
        return referenced_resources

    def resolve_reference_path(self, ref):
        filename = ref['filename']
        candidates = []
        for key, manifest_info in self.manifest_paths.items():
            full_path = manifest_info['full_path'] / filename
            if full_path.exists():
                candidates.append(full_path)
        if candidates:
            return candidates[0]
        for key, manifest_info in self.manifest_paths.items():
            for prefix in manifest_info['search_prefixes']:
                candidate = self.project_root / prefix / manifest_info['path'] / filename
                if candidate.exists():
                    candidates.append(candidate)
        return candidates[0] if candidates else None

    def is_ignored_missing_filename(self, filename):
        if filename in OUTPUT_FILENAME_PATTERNS:
            return True
        if filename in FAKE_TEST_FILENAMES:
            return True
        if filename in TEST_STRING_ONLY_FILENAMES:
            return True
        for prefix in OUTPUT_FILENAME_PREFIXES:
            if filename.startswith(prefix):
                return True
        return False

    def check_referenced_resources(self, referenced_resources):
        actual_files_map = {}
        for dirpath, files in self.collect_actual_files().items():
            for f in files:
                if f not in actual_files_map:
                    actual_files_map[f] = dirpath + '/' + f
        missing = []
        for ref in referenced_resources:
            filename = ref['filename']
            if self.is_ignored_missing_filename(filename):
                continue
            resolved = self.resolve_reference_path(ref)
            if not resolved:
                if filename in actual_files_map:
                    resolved = self.project_root / actual_files_map[filename]
                else:
                    if ref['kind'] == 'string_literal' and Path(filename).is_absolute():
                        continue
                    missing.append(ref)
                    continue
            ref_type = self.classify_extension(filename)
            if ref_type and resolved:
                parent_dir = str(resolved.parent.relative_to(self.project_root))
                allowed = None
                for hint_path, types in RESOURCE_TYPE_HINTS.items():
                    if parent_dir.startswith(hint_path):
                        allowed = types
                        break
                if allowed and ref_type not in allowed:
                    self.warning(
                        f"{ref['source']}:{ref['line']}: "
                        f"File '{filename}' (type: {ref_type}) may not match expected "
                        f"types for directory '{parent_dir}' (expected: {sorted(allowed)})",
                        strict_error=True,
                    )
        for ref in missing:
            self.error(
                f"{ref['source']}:{ref['line']}: "
                f"Referenced resource file not found: '{ref['filename']}' "
                f"(via {ref['name']})"
            )

    def check_texture_packer_spritesheet(self):
        tp_dir = self.resources_dir / 'SharedSupport' / 'TexturePacker' / 'nomlib'
        if not tp_dir.exists():
            self.error(f'TexturePacker directory not found: {tp_dir.relative_to(self.project_root)}')
            return
        spritesheet_json = tp_dir / 'spritesheet.json'
        exporter_xml = tp_dir / 'exporter.xml'
        if not spritesheet_json.exists():
            self.error(f'TexturePacker spritesheet.json not found: {spritesheet_json.relative_to(self.project_root)}')
        if not exporter_xml.exists():
            self.warning(f'TexturePacker exporter.xml not found: {exporter_xml.relative_to(self.project_root)}')
        if spritesheet_json.exists():
            try:
                with open(spritesheet_json, 'r', encoding='utf-8') as f:
                    content = f.read()
                    if '{{%' in content or '{{' in content:
                        pass
                    else:
                        json.loads(content)
            except json.JSONDecodeError as e:
                if '{{%' not in content:
                    self.error(f'TexturePacker spritesheet.json parse issue: {e}')

    def check_input_devices(self):
        input_devices_dir = self.resources_dir / 'SharedSupport' / 'InputDevices'
        if not input_devices_dir.exists():
            self.error(f'InputDevices directory not found: {input_devices_dir.relative_to(self.project_root)}')
            return
        gamecontrollerdb = input_devices_dir / 'gamecontrollerdb.txt'
        if not gamecontrollerdb.exists():
            self.error(f'gamecontrollerdb.txt not found: {gamecontrollerdb.relative_to(self.project_root)}')
            return
        try:
            with open(gamecontrollerdb, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            valid_entries = 0
            for line in lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                if ',' in line and line.count(',') >= 3:
                    valid_entries += 1
            if valid_entries == 0:
                self.warning(f'gamecontrollerdb.txt has no valid controller entries')
        except Exception as e:
            self.error(f'Failed to read gamecontrollerdb.txt: {e}')
        input_devices_json = self.resources_dir / 'SharedSupport' / 'InputDevices.json'
        if input_devices_json.exists():
            data = self.parse_manifest(input_devices_json)
            if data:
                self.check_manifest(input_devices_json, data)
        else:
            self.warning(f'InputDevices.json manifest not found')

    def check_duplicate_resources(self):
        actual_files = self.collect_actual_files()
        file_locations = defaultdict(list)
        for dirpath, files in actual_files.items():
            for f in files:
                file_locations[f].append(dirpath)
        for filename, locations in sorted(file_locations.items()):
            if len(locations) > 1:
                unique_locs = set(locations)
                if len(unique_locs) > 1:
                    self.warning(
                        f'Duplicate resource filename: {filename} found in '
                        f'multiple locations: {sorted(unique_locs)}',
                        strict_error=True,
                    )

    def check_unreferenced_resources(self):
        referenced = set()
        refs = self.scan_source_for_resources()
        for r in refs:
            referenced.add(r['filename'])
        actual_files = self.collect_actual_files()
        unreferenced = []
        exclude_dir_patterns = (
            'ImageDiffTest',
            'VisualUnitTest',
        )
        exclude_files = {
            'LICENSE', 'README', 'source.url', 'exporter.xml',
            'spritesheet.json', 'gamecontrollerdb.txt',
            'results.css', 'results.js', 'tags.xml',
        }
        for dirpath, files in actual_files.items():
            if any(p in dirpath for p in exclude_dir_patterns):
                continue
            for f in sorted(files):
                if f.startswith('.'):
                    continue
                if f in exclude_files:
                    continue
                if f.endswith(('.json',)):
                    is_test_manifest = (
                        dirpath.startswith('Resources/tests')
                        and dirpath.count('/') <= 3
                    )
                    if is_test_manifest:
                        continue
                if f not in referenced:
                    unreferenced.append(f'{dirpath}/{f}')
        for f in sorted(unreferenced):
            self.warning(f'Potentially unreferenced resource: {f}')

    def run(self):
        mode = 'STRICT' if self.strict else 'normal'
        print(f'Checking resources in {self.project_root} (mode: {mode})')
        print()
        print('== Checking manifest files ==')
        manifests = self.find_manifest_files()
        for m in manifests:
            print(f'  Manifest: {m.relative_to(self.project_root)}')
            data = self.parse_manifest(m)
            if data:
                self.check_manifest(m, data)
        print()
        print('== Checking TexturePacker spritesheet ==')
        self.check_texture_packer_spritesheet()
        print()
        print('== Checking InputDevices ==')
        self.check_input_devices()
        print()
        print('== Scanning source files for resource references ==')
        refs = self.scan_source_for_resources()
        print(f'  Found {len(refs)} resource references in source code')
        self.check_referenced_resources(refs)
        print()
        print('== Checking for duplicate resources ==')
        self.check_duplicate_resources()
        print()
        print('== Checking for unreferenced resources ==')
        self.check_unreferenced_resources()
        print()
        if self.warnings:
            print(f'{len(self.warnings)} warning(s):')
            for w in self.warnings:
                print(f'  {w}')
            print()
        if self.errors:
            print(f'{len(self.errors)} error(s):')
            for e in self.errors:
                print(f'  {e}')
            print()
            print('FAILED: Resource validation failed.')
            return 1
        print('PASSED: All resource checks passed.')
        return 0

def main():
    import argparse
    parser = argparse.ArgumentParser(
        description='Validate nomlib resources, manifests and source references.'
    )
    parser.add_argument(
        '--strict', action='store_true',
        help='Fail on warnings (duplicates, type mismatches, unreferenced files).'
    )
    args = parser.parse_args()
    project_root = Path(__file__).resolve().parent.parent
    checker = ResourceChecker(project_root, strict=args.strict)
    sys.exit(checker.run())

if __name__ == '__main__':
    main()

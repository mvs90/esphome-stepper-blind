#!/usr/bin/env python3
"""Exercise ESPHome schema and code generation without network or real credentials."""
from pathlib import Path
import base64
import shutil
import subprocess
import sys
import tempfile
from ruamel.yaml import YAML

root = Path(__file__).resolve().parent.parent
yaml = YAML()
with tempfile.TemporaryDirectory(prefix='stepper-blind-config-') as directory:
    temp = Path(directory)
    shutil.copytree(root / 'components', temp / 'components')
    (temp / 'secrets.yaml').write_text(
        'wifi_ssid: validation-network\nwifi_password: validation-password\n'
        'ota_password: validation-ota-password\napi_encryption_key: "'
        + base64.b64encode(bytes(range(32))).decode() + '"\n'
    )
    cases = [(1, True, True), (-1, False, True), (90000, True, True),
             (-90000, False, True), (12345, True, True), (0, True, False),
             (90001, True, False), (-90001, True, False), (1.5, True, False)]
    for endpoint, number, valid in cases:
        config = yaml.load(root / 'example.yaml')
        config['external_components'][0]['source'] = {'type': 'local', 'path': 'components'}
        config['cover'][0]['closed_position'] = endpoint
        if not number:
            config['cover'][0].pop('endstop')
        filename = temp / 'example.yaml'
        yaml.dump(config, filename)
        result = subprocess.run([sys.executable, '-m', 'esphome', 'compile', str(filename),
                                 '--only-generate'], capture_output=True, text=True)
        if (result.returncode == 0) != valid:
            raise AssertionError(f'endpoint={endpoint}: {result.stdout}\n{result.stderr}')
        print(f'PASS schema/codegen endpoint={endpoint}, number={number}, valid={valid}')

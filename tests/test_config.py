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

    speed_cases = [
        (250, None, True), (250, 100, True), (250, 250, True),
        (250, 1, True), (250, 0, False), (250, -1, False),
        (250, 251, False), (250, "nan", False), (250, float("inf"), False),
        (250, "100 steps/s", True), (100, 150, False),
        (500, 400, True), (0.5, None, False),
    ]
    for maximum, initial, valid in speed_cases:
        config = yaml.load(root / 'example.yaml')
        config['external_components'][0]['source'] = {'type': 'local', 'path': 'components'}
        config['stepper'][0]['max_speed'] = maximum
        speed = config['cover'][0]['speed']
        speed.pop('initial_value', None)
        if initial is not None:
            speed['initial_value'] = initial
        filename = temp / 'speed.yaml'
        yaml.dump(config, filename)
        result = subprocess.run([sys.executable, '-m', 'esphome', 'compile', str(filename),
                                 '--only-generate'], capture_output=True, text=True)
        if (result.returncode == 0) != valid:
            raise AssertionError(f'maximum={maximum}, initial={initial}: {result.stdout}\n{result.stderr}')
        print(f'PASS speed schema max={maximum}, initial={initial}, valid={valid}')
    config['cover'][0].pop('speed')
    yaml.dump(config, filename)
    subprocess.run([sys.executable, '-m', 'esphome', 'compile', str(filename),
                    '--only-generate'], check=True, capture_output=True)
    print('PASS optional speed block omitted')

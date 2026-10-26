#!/usr/bin/env python3
"""Full ESP8266 builds against local sources with publicly known test credentials."""
from pathlib import Path
import base64
import shutil
import subprocess
import sys
import tempfile
from ruamel.yaml import YAML

root = Path(__file__).resolve().parent.parent
yaml = YAML()
with tempfile.TemporaryDirectory(prefix='stepper-blind-firmware-') as directory:
    temp = Path(directory)
    shutil.copytree(root / 'components', temp / 'components')
    (temp / 'secrets.yaml').write_text(
        'wifi_ssid: validation-network\nwifi_password: validation-password\n'
        'ota_password: validation-ota-password\napi_encryption_key: "'
        + base64.b64encode(bytes(range(32))).decode() + '"\n'
    )
    for source, endpoint, speed_control in [(root / 'example.yaml', 10000, True),
                             (root / 'examples/schlafzimmer-jalousie-fenster4.yaml', -90000, True),
                             (root / 'example.yaml', 10000, False)]:
        config = yaml.load(source)
        config['external_components'][0]['source'] = {'type': 'local', 'path': 'components'}
        config['cover'][0]['closed_position'] = endpoint
        if not speed_control:
            config['cover'][0].pop('speed', None)
        target = temp / source.name
        yaml.dump(config, target)
        subprocess.run([sys.executable, '-m', 'esphome', 'compile', str(target)], check=True)

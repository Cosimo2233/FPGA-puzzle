"""Compatibility entry for the canonical bundle builder."""
from pathlib import Path
import runpy
if __name__ == '__main__':
    runpy.run_path(str(Path(__file__).resolve().parents[1]/'package_hardware.py'),run_name='__main__')

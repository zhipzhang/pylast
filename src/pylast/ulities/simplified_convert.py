#!/usr/bin/env python3
import subprocess
import sys
import os

def simplified_convert():
    # Get the path to the C++ executable
    # This assumes the executable is installed alongside the Python package
    package_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    executable = os.path.join(package_dir, "Simplified_Convert")
    
    
    # Forward all command line arguments to the C++ executable
    try:
        result = subprocess.run([executable] + sys.argv[1:], check=True)
        return result.returncode
    except subprocess.CalledProcessError as e:
        print(f"Error running simplified_convert: {e}", file=sys.stderr)
        return e.returncode

if __name__ == "__main__":
    sys.exit(simplified_convert())

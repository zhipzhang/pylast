from .helper import initialize_logger, shutdown_logger
from pathlib import Path

try:
    from .io import PrototypeEventSource
    _package_dir = Path(__file__).parent
    _prototype_camera_path = _package_dir / "config"
    if _prototype_camera_path.exists():
        PrototypeEventSource.set_config_path(str(_prototype_camera_path))
    else:
        raise FileNotFoundError(f"Prototype camera path not found: {_prototype_camera_path}")
except ImportError:
    pass
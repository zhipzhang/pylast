import os
import subprocess
import argparse
from pathlib import Path
from .corsika_inputgen import CorsikaAutoInput

class CorsikaRun:
    def __init__(self, corsika_path, template_path=None):
        self.corsika_path = Path(corsika_path)
        if template_path is None:
            # Set default template path to be in the same directory as this script
            self.template_path = Path(__file__).parent / "template.input"
        else:
            self.template_path = Path(template_path)
        self.exe_name = self.corsika_path.name

    def setup_work_directory(self, run_number):
        self.work_dir = Path(f"run{run_number:06d}")
        self.work_dir.mkdir(parents=True, exist_ok=True)
        
        cmd = f"(cd {self.work_dir} && for f in GLAUBTAR.DAT NUCNUCCS NUCLEAR.BIN " \
              f"VENUSDAT QGSDAT01 SECTNU qgsdat-II-03 sectnu-II-03 qgsdat-II-04 sectnu-II-04 " \
              f"UrQMD-1.3.1-xs.dat tables.dat; do " \
              f"if [ -f {self.corsika_path.parent}/$f ]; then ln -s {self.corsika_path.parent}/$f .; fi; done && " \
              f"for f in epos.inics.lhc epos.iniev epos.ini1b epos.inirj.lhc epos.initl epos.param; do " \
              f"  if [ -f {self.corsika_path.parent}/$f ]; then ln -s {self.corsika_path.parent}/$f .; " \
              f"  elif [ -f {self.corsika_path.parent}/../epos/$f ]; then ln -s {self.corsika_path.parent}/../epos/$f .; fi; done && " \
              f"ln -s {self.corsika_path.parent}/EGS* . && ln -s {self.corsika_path.parent}/atmprof*.dat . && " \
              f"ln -s {self.corsika_path} corsika_executable)"
        
        subprocess.run(cmd, shell=True, check=True)

    def generate_input_card(self, **kwargs):
        auto_input = CorsikaAutoInput(template_path=self.template_path, **kwargs)
        auto_input.generate_input_card()
        input_file = self.work_dir / "corsika.input"
        auto_input.write_output(input_file)
        return input_file

    def run_corsika(self, input_file):
        output_file = self.work_dir / "corsika.output"
        corsika_executable = self.work_dir / "corsika_executable"
        
        with output_file.open("w") as out:
            subprocess.run(
                [str(corsika_executable)],
                cwd=str(self.work_dir),
                stdin=input_file.open("r"),
                stdout=out,
                stderr=subprocess.STDOUT,
                check=True
            )

    def execute(self, **kwargs):
        run_number = kwargs.get('run_number')
        if run_number is None:
            raise ValueError("run_number must be provided")
        
        self.setup_work_directory(run_number)
        input_file = self.generate_input_card(**kwargs)
        self.run_corsika(input_file)

def main():
    parser = argparse.ArgumentParser(description="Run CORSIKA simulation")
    parser.add_argument("--corsika-path", required=True, help="Path to CORSIKA executable")
    parser.add_argument("--template", help="Path to template input file")
    parser.add_argument("--run-number", type=int, required=True, help="Run number")
    parser.add_argument("--config-path", help="Path to configuration directory")
    parser.add_argument("--keep-seeds", action="store_true", help="Keep seeds from template")
    parser.add_argument("--energy-range", help="Energy range for simulation")
    parser.add_argument("--particle-type", help="Particle type for simulation")
    parser.add_argument("--atmosphere", type=int, help="Atmosphere model number")

    args = parser.parse_args()

    runner = CorsikaRun(args.corsika_path, args.template)
    runner.execute(
        run_number=args.run_number,
        config_path=args.config_path,
        keep_seeds=args.keep_seeds,
        energy_range=args.energy_range,
        particle_type=args.particle_type,
        atmosphere_number=args.atmosphere
    )

if __name__ == "__main__":
    main()

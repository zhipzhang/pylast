"""
corsika_inputgen.py

This script is used to automatically generate input files for CORSIKA simulations.
"""
import random
import socket
import argparse
import os
from pathlib import Path


def generate_seed():
    return [
        random.randint(1, 900000000),
        random.randint(0, 999999),
    ]

class CorsikaAutoInput:
    def __init__(self, template_path="template.input", config_path=None, run_number=None, 
                 keep_seeds=False, energy_range=None, particle_type=None, atmosphere_number=None):
        self.template_path    = Path(template_path)
        self.config_path      = Path(config_path) if config_path else Path.cwd()
        self.run_conf_file    = self.config_path / "run.conf"
        self.fixed_run_number = run_number
        self.keep_seeds       = keep_seeds
        self.energy_range     = energy_range
        self.particle_type    = particle_type
        self.atmosphere_number = atmosphere_number
        self.input_card       = []
        self.blank_lines      = []  # Store the blank lines from the template

    def load_template(self):
        with open(self.template_path, 'r') as template_file:
            lines = template_file.readlines()
            # Store blank lines and their positions
            for i, line in enumerate(lines):
                if not line.strip():
                    self.blank_lines.append(i)
                else:
                    parts = line.strip().split()
                    if len(parts) > 1:
                        key = parts[0]
                        value = ' '.join(parts[1:])
                        self.input_card.append((key, value))
                    else:
                        self.input_card.append((parts[0], ''))

    def generate_input_card(self):
        self.load_template()
        self._set_run_number()
        self._set_host_user()
        self._set_seeds()
        self._set_energy_range()
        self._set_particle_type()
        self._set_atmosphere()

    def _set_run_number(self):
        if self.fixed_run_number is not None:
            run_number = self.fixed_run_number
        else:
            run_number = self._get_next_run_number()
        self._update_or_append('RUNNR', str(run_number))

    def _get_next_run_number(self):
        if not self.run_conf_file.exists():
            run_number = 1
        else:
            with open(self.run_conf_file, 'r') as f:
                run_number = int(f.read().strip()) + 1

        with open(self.run_conf_file, 'w') as f:
            f.write(str(run_number))

        return run_number

    def _set_host_user(self):
        self._update_or_append('HOST', socket.gethostname())
        self._update_or_append('USER', os.getlogin())

    def _set_seeds(self):
        if not self.keep_seeds:
            self.input_card = [item for item in self.input_card if not item[0].startswith('SEED')]
            
            for _ in range(4):
                seed = generate_seed()
                self.input_card.append(('SEED', f'{seed[0]:9d} {seed[1]:6d} 0'))

    def _set_energy_range(self):
        if self.energy_range:
            self._update_or_append('ERANGE', self.energy_range)

    def _set_particle_type(self):
        if self.particle_type:
            self._update_or_append('PRMPAR', self.particle_type)

    def _set_atmosphere(self):
        if self.atmosphere_number:
            self._update_or_append('ATMOSPHERE', f"{self.atmosphere_number} F")

    def _update_or_append(self, key, value):
        for i, (k, v) in enumerate(self.input_card):
            if k == key:
                self.input_card[i] = (key, value)
                return
        self.input_card.append((key, value))

    def write_output(self, output_file=None):
        # Reinsert blank lines
        output_lines = []
        input_card_index = 0
        for i in range(len(self.input_card) + len(self.blank_lines)):
            if i in self.blank_lines:
                output_lines.append('')
            else:
                key, value = self.input_card[input_card_index]
                output_lines.append(f"{key:<12} {value}")
                input_card_index += 1
        
        output = "\n".join(output_lines)
        if output_file:
            with open(output_file, 'w') as f:
                f.write(output)
        else:
            print(output)

def main():
    parser = argparse.ArgumentParser(description="Generate CORSIKA input card")
    parser.add_argument("--template",     default="template.input", help="Path to template input file")
    parser.add_argument("--config-path",  help="Path to configuration directory")
    parser.add_argument("--run-number",   type=int, help="Fixed run number")
    parser.add_argument("--keep-seeds",   action="store_true", help="Keep seeds from template")
    parser.add_argument("--energy-range", help="Energy range for simulation")
    parser.add_argument("--particle-type", help="Particle type for simulation")
    parser.add_argument("--atmosphere",   type=int, help="Atmosphere model number")
    parser.add_argument("--output",       help="Output file path (if not provided, print to stdout)")

    args = parser.parse_args()

    auto_input = CorsikaAutoInput(
        template_path    = args.template,
        config_path      = args.config_path,
        run_number       = args.run_number,
        keep_seeds       = args.keep_seeds,
        energy_range     = args.energy_range,
        particle_type    = args.particle_type,
        atmosphere_number = args.atmosphere
    )

    auto_input.generate_input_card()
    auto_input.write_output(args.output)

if __name__ == "__main__":
    main()

from pylast.helper import make_regular_histogram, make_regular_histogram2d, make_regular_profile
import numpy as np
from typing import Dict, Tuple, Optional
from collections import OrderedDict


class BaseHistogramProfile:
    def __init__(self, base_name: str, energy_range: np.ndarray, offset_angle_range: np.ndarray, 
                 impact_parameter_range: np.ndarray = np.linspace(0, 600, 30)):
        self.base_name = base_name
        self.energy_range = energy_range
        self.offset_angle_range = offset_angle_range
        self.impact_parameter_range = impact_parameter_range
        self.histograms = OrderedDict()
        self.initialize_histograms()
    
    def initialize_histograms(self):
        """Initialize profile histograms for each energy and offset angle bin"""
        # Create profile histograms for each energy bin and offset angle bin
        for i in range(len(self.energy_range) - 1):
            for j in range(len(self.offset_angle_range) - 1):
                # Get the bin edges
                energy_min = self.energy_range[i]
                energy_max = self.energy_range[i + 1]
                offset_min = self.offset_angle_range[j]
                offset_max = self.offset_angle_range[j + 1]
                
                # Create a key for this energy/offset bin
                key = self._make_key(i, j)
                
                # Create a profile histogram with impact parameter on x-axis
                min_impact = self.impact_parameter_range[0]
                max_impact = self.impact_parameter_range[-1]
                num_bins = len(self.impact_parameter_range) - 1
                
                # Create a profile histogram for this energy/offset bin
                self.histograms[key] = make_regular_profile(min_impact, max_impact, num_bins)
    
    def _make_key(self, energy_bin: int, offset_bin: int) -> str:
        """Create a unique key for a histogram based on energy and offset angle bin indices"""
        energy_min = self.energy_range[energy_bin]
        energy_max = self.energy_range[energy_bin + 1]
        offset_min = self.offset_angle_range[offset_bin]
        offset_max = self.offset_angle_range[offset_bin + 1]
        
        # Format energy values with fixed width and leading zeros for proper sorting
        # Use 8 digits for energy to ensure proper ordering (e.g., 0000.10, 0100.00)
        
        # Format offset values with fixed width and leading zeros
        
        return f"{self.base_name}_energy_{energy_min:06.2f}_{energy_max:06.2f}_offset_{offset_min}_{offset_max}"
    def fill(self, energy: float, offset_angle: float, impact_parameter: float, value: float, weight: float = 1.0):
        """Fill the appropriate histogram based on energy and offset angle"""
        # Find the bin indices for energy and offset angle
        energy_bin = self._get_bin_index(energy, self.energy_range)
        offset_bin = self._get_bin_index(offset_angle, self.offset_angle_range)
        
        # Skip if the values are outside our binning range
        if energy_bin < 0 or offset_bin < 0:
            return
        
        # Get the key for this histogram
        key = self._make_key(energy_bin, offset_bin)
        
        # Fill the histogram
        self.histograms[key].fill(impact_parameter, value, weight)
    
    def _get_bin_index(self, value: float, bin_edges: np.ndarray) -> int:
        """Get the bin index for a value given bin edges"""
        for i in range(len(bin_edges) - 1):
            if bin_edges[i] <= value < bin_edges[i + 1]:
                return i
        return -1  # Value is outside the binning range
    
    def get_histogram(self, energy_bin: int, offset_bin: int):
        """Get the histogram for a specific energy and offset angle bin"""
        key = self._make_key(energy_bin, offset_bin)
        return self.histograms.get(key)
    
    def get_all_histograms(self) -> Dict[str, object]:
        """Get all histograms"""
        return self.histograms

    
class BaseHistogram1D:
    def __init__(self, base_name: str, offset_angle_range: np.ndarray, energy_range:np.ndarray, x_range:np.ndarray):
        self.base_name = base_name
        self.offset_angle_range = offset_angle_range
        self.energy_range = energy_range
        self.x_range = x_range
        self.histograms = OrderedDict()
        self.initialize_histograms()
    def initialize_histograms(self):
        """Initialize 2D histograms for each offset angle bin"""
        for i in range(len(self.offset_angle_range) - 1):
            for j in range(len(self.energy_range) - 1):
                key = self._make_key(i, j)
                self.histograms[key] = make_regular_histogram(min = self.x_range[0], max = self.x_range[-1], bins = len(self.x_range) - 1)
    def _make_key(self, offset_angle_bin: int, energy_bin: int) -> str:
        """Create a unique key for a histogram based on offset angle bin index"""
        offset_min = self.offset_angle_range[offset_angle_bin]
        offset_max = self.offset_angle_range[offset_angle_bin + 1]
        energy_min = self.energy_range[energy_bin]
        energy_max = self.energy_range[energy_bin + 1]
        # Format with leading zeros for both energy and offset values
        return f"{self.base_name}_energy_{energy_min:06.2f}_{energy_max:06.2f}_offset_{offset_min}_{offset_max}"
    def _get_bin_index(self, value: float, bin_edges: np.ndarray) -> int:
        """Get the bin index for a value given bin edges"""
        for i in range(len(bin_edges) - 1):
            if bin_edges[i] <= value < bin_edges[i + 1]:
                return i
        return -1  # Value is outside the binning range
    def fill(self, offset_angle: float, energy: float, x: float, weight: float = 1.0):
        """Fill the appropriate histogram based on offset angle and x and y values"""
        offset_bin = self._get_bin_index(offset_angle, self.offset_angle_range)
        energy_bin = self._get_bin_index(energy, self.energy_range)
        if offset_bin < 0 or energy_bin < 0:
            return
        key = self._make_key(offset_bin, energy_bin)
        self.histograms[key].fill(x, weight)
    def get_histogram(self, offset_angle_bin: int, energy_bin: int):
        """Get the histogram for a specific offset angle bin"""
        key = self._make_key(offset_angle_bin, energy_bin)
        return self.histograms.get(key)
    def get_all_histograms(self) -> Dict[str, object]:
        """Get all histograms"""
        return self.histograms
        
        
                    

"""
Helper script to optimize the theta_cuts, gh_score cuts to maximize the sensitivity.
"""

from astropy.table import QTable
from pyirf.sensitivity import relative_sensitivity
import astropy.units as u
import numpy as np


def optimize_cuts(
    gamma_data: QTable, proton_data: QTable, reco_energy_bins: np.ndarray
):
    """
    Optimize theta and gh_score cuts for each energy bin.

    Parameters
    ----------
    gamma_data : QTable
        Gamma events with columns: reco_energy, theta, gh_score, weights.
    proton_data : QTable
        Proton events with columns: reco_energy, gh_score, weights, true_source_fov_offset.
    reco_energy_bins : np.ndarray
        Energy bin edges (Quantity). If length n+1, yields n bins [bin[i], bin[i+1]).

    Returns
    -------
    QTable
        Results per energy bin: energy_low, energy_high, theta_cut, gh_cut, sensitivity.
    """
    gamma_reco_energy = gamma_data["reco_energy"]
    proton_reco_energy = proton_data["reco_energy"]

    energy_lows = []
    energy_highs = []
    theta_cuts = []
    gh_cuts = []
    sensitivities = []

    for i in range(len(reco_energy_bins) - 1):
        e_low = reco_energy_bins[i]
        e_high = reco_energy_bins[i + 1]

        gamma_mask = (gamma_reco_energy >= e_low) & (gamma_reco_energy < e_high)
        proton_mask = (proton_reco_energy >= e_low) & (proton_reco_energy < e_high)

        gamma_bin = gamma_data[gamma_mask]
        proton_bin = proton_data[proton_mask]

        if len(gamma_bin) == 0 or len(proton_bin) == 0:
            best_theta, best_gh_cut, best_sensitivity = np.nan, np.nan, np.nan
        else:
            best_theta, best_gh_cut, best_sensitivity = (
                _optimize_cuts_single_energy_bin(gamma_bin, proton_bin)
            )

        energy_lows.append(e_low)
        energy_highs.append(e_high)
        theta_cuts.append(best_theta)
        gh_cuts.append(best_gh_cut)
        sensitivities.append(best_sensitivity)

    return QTable({
        "energy_low": energy_lows,
        "energy_high": energy_highs,
        "theta_cut": np.array(theta_cuts) * u.Unit("deg"),
        "gh_cut": np.array(gh_cuts),
        "sensitivity": np.array(sensitivities),
    })

def _optimize_cuts_single_energy_bin(gamma_data: QTable, proton_data: QTable):
    """
    Vectorized optimization of cut conditions, calculating the sensitivity for all combinations in one go.

    Returns
    -------
    best_theta : float
        Best theta cut (degrees)
    best_gh_cut : float
        Best gh_score threshold (actual value)
    best_sensitivity : float
        Minimum sensitivity value
    """
    ALPHA = 0.2

    # Precompute some constants
    max_proton_offset = np.max(proton_data["reco_source_fov_offset"])
    simulation_solid_angle = 2 * np.pi * (1 - np.cos(max_proton_offset))

    # Pre-extract numpy arrays for faster operations
    gamma_theta = gamma_data["theta"].to(u.Unit("deg")).value
    gamma_gh = np.asarray(gamma_data["gh_score"])
    gamma_w = np.asarray(gamma_data["weights"])

    proton_gh = np.asarray(proton_data["gh_score"])
    proton_w = np.asarray(proton_data["weights"])

    # Scanning grids for theta and gh_score
    theta_cuts = np.arange(0.05, 0.3, 0.01)
    gh_ratios = np.arange(0.2, 0.95, 0.01)

    # Store parameters and counts for each combination
    param_list = []  # [(theta_cut, gh_cut_value), ...]
    n_ons = []
    n_offs = []

    for theta_cut in theta_cuts:
        # Theta cut mask
        theta_mask = gamma_theta < theta_cut
        n_gamma_theta = np.count_nonzero(theta_mask)

        if n_gamma_theta == 0:
            continue

        gamma_gh_sel = gamma_gh[theta_mask]
        gamma_w_sel = gamma_w[theta_mask]

        # Calculate solid angle scaling factor (includes ALPHA normalization)
        cut_solid_angle = 2 * np.pi * (1 - np.cos(np.radians(theta_cut)))
        scale_factor = cut_solid_angle / simulation_solid_angle / ALPHA

        # Compute all gh_score cut values for the current theta cut
        gh_cut_values = np.percentile(gamma_gh_sel, gh_ratios * 100)

        # Calculate protons passing each gh_cut (using boolean sum)
        for gh_cut in gh_cut_values:
            # N_on: number of gamma passing the theta and gh_score cut
            n_on = np.sum(gamma_w_sel[gamma_gh_sel < gh_cut])

            # N_off: number of protons passing gh_score cut, scaled by solid angle factor
            n_proton_pass = np.sum(proton_w[proton_gh < gh_cut])
            n_off = n_proton_pass * scale_factor

            n_ons.append(n_on)
            n_offs.append(n_off)
            param_list.append((theta_cut, gh_cut))

    if not n_ons:
        return np.nan, np.nan, np.nan

    # Vectorized sensitivity calculation for all candidates
    n_ons = np.array(n_ons)
    n_offs = np.array(n_offs)
    sensitivities = relative_sensitivity(n_ons, n_offs, ALPHA)

    # Find minimum (ignoring nan)
    valid_mask = ~np.isnan(sensitivities)
    if not np.any(valid_mask):
        return np.nan, np.nan, np.nan

    best_idx = np.nanargmin(sensitivities)
    best_theta, best_gh_cut = param_list[best_idx]
    best_sensitivity = sensitivities[best_idx]

    return best_theta, best_gh_cut, best_sensitivity

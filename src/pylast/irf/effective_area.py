from pyirf.spectral import CRAB_HEGRA
import astropy.units as u  # pylint: disable=all
import numpy as np
from astropy.table import QTable


def _compute_expected_events(
    spectral_flux,
    energy_low: u.Quantity,
    energy_high: u.Quantity,
    time: u.Quantity = 50 * u.Unit("hour"),
) -> u.Quantity:
    """
    Expected number of events per unit area (cm^-2) in the energy bin for given observation time.
    """
    e_low = energy_low.to(u.TeV).value
    e_high = energy_high.to(u.TeV).value
    # 积分网格，保证足够的精度
    energy_bins = np.logspace(np.log10(e_low), np.log10(e_high), 100) * u.TeV
    flux = spectral_flux(energy_bins)
    return (np.trapz(flux, energy_bins)* time).to("cm^-2")


def compute_effective_area(
    gamma_data: QTable, cuts: QTable, scatter_radius: u.Quantity, time: u.Quantity = 50 * u.hour
) -> QTable:
    """
    Compute effective area for each energy bin and cut condition.

    Uses CRAB_HEGRA spectrum: expected events per cm^2 in 50 h per bin.
    Cuts are applied on reco_energy, but area is binned and evaluated on true_energy.
    """
    scatter_area = np.pi * scatter_radius**2
    
    # 提取需要的列：包括 reco_energy 和 true_energy
    reco_energy_val = gamma_data["reco_energy"].to(u.TeV).value
    true_energy_val = gamma_data["true_energy"].to(u.TeV).value
    theta = gamma_data["theta"].to(u.deg).value
    gh_score = np.asarray(gamma_data["gh_score"])
    
    if "weights" in gamma_data.colnames:
        weights = np.asarray(gamma_data["weights"])
    else:
        weights = np.ones(len(gamma_data))

    energy_lows = np.asarray(cuts["energy_low"].to(u.TeV).value)
    energy_highs = np.asarray(cuts["energy_high"].to(u.TeV).value)
    theta_cuts = np.asarray(cuts["theta_cut"].to(u.deg).value)
    gh_cuts = np.asarray(cuts["gh_cut"])

    # === 第一步：基于 Reco Energy 全局应用 Cuts，打布尔标签 ===
    pass_gh = np.zeros(len(gamma_data), dtype=bool)
    pass_all = np.zeros(len(gamma_data), dtype=bool)

    for i in range(len(cuts)):
        # 找到属于当前 Reco Energy Bin 的事件
        reco_mask = (reco_energy_val >= energy_lows[i]) & (reco_energy_val < energy_highs[i])
        
        # 在这些事件中，通过 GH cut 的事件
        gh_mask_this_bin = reco_mask & (gh_score < gh_cuts[i])
        pass_gh |= gh_mask_this_bin
        
        # 同时通过 GH cut 和 Theta cut 的事件
        pass_all |= gh_mask_this_bin & (theta < theta_cuts[i])


    # === 第二步：在 True Energy Bin 下计算预期事件数和实际通过数 ===
    result_no_cuts = []
    result_gh_cuts = []
    result_all_cuts = []

    # 注意：通常 IRF 的有效面积能量网格和 Cut 能量网格可以一致，这里复用 cuts 表的能量区间作为 True Energy 的 Bin
    for i in range(len(cuts)):
        e_low, e_high = energy_lows[i], energy_highs[i]
        
        # 预期数量必须在 True Energy 区间内对能谱积分
        expected = _compute_expected_events(CRAB_HEGRA, e_low * u.TeV, e_high * u.TeV, time)
        expected_val = (expected * scatter_area).to("")

        if expected_val <= 0:
            result_no_cuts.append(np.nan)
            result_gh_cuts.append(np.nan)
            result_all_cuts.append(np.nan)
            continue

        # 当前 True Energy Bin 内的事件
        true_mask = (true_energy_val >= e_low) & (true_energy_val < e_high)
        w_bin = weights[true_mask]
        
        # 提取当前 True Energy Bin 内事件的 Cut 通过状态
        mask_gh_true = pass_gh[true_mask]
        mask_all_true = pass_all[true_mask]

        # No cuts (只要在这个 True Energy 里的所有探测到的事件)
        actual_no = np.sum(w_bin)
        area_no = (actual_no / expected_val) * scatter_area if expected_val > 0 else np.nan
        result_no_cuts.append(area_no.to(u.m**2).value if not np.isnan(area_no) else np.nan)

        # GH cuts only
        actual_gh = np.sum(w_bin[mask_gh_true])
        area_gh = (actual_gh / expected_val) * scatter_area if expected_val > 0 else np.nan
        result_gh_cuts.append(area_gh.to(u.m**2).value if not np.isnan(area_gh) else np.nan)

        # All cuts
        actual_all = np.sum(w_bin[mask_all_true])
        area_all = (actual_all / expected_val) * scatter_area if expected_val > 0 else np.nan
        result_all_cuts.append(area_all.to(u.m**2).value if not np.isnan(area_all) else np.nan)

    return QTable(
        {
            "energy_low": cuts["energy_low"],
            "energy_high": cuts["energy_high"],
            "effective_area_no_cuts": result_no_cuts * u.m**2,
            "effective_area_with_gh_cuts": result_gh_cuts * u.m**2,
            "effective_area_with_all_cuts": result_all_cuts * u.m**2,
        }
    )
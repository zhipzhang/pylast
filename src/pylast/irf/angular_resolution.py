from typing import Optional
from astropy.table import QTable
import astropy.units as u #pylint: disable=all
import numpy as np


def _percentile_68(theta: np.ndarray, weights: Optional[np.ndarray] = None) -> float:
    """
    68% containment of theta.
    If weights is None or not provided, uses np.quantile (unweighted).
    If weights is provided, uses weighted quantile (inverted_cdf).
    Returns np.nan if empty or zero total weight.
    """
    if len(theta) == 0:
        return np.nan
    if weights is None or np.sum(weights) <= 0:
        return float(np.quantile(theta, 0.68))
    order = np.argsort(theta)
    theta_sorted = theta[order]
    cumsum = np.cumsum(weights[order])
    total = cumsum[-1]
    if total <= 0:
        return np.nan
    idx = np.searchsorted(cumsum, 0.68 * total, side="left")
    if idx >= len(theta_sorted):
        idx = len(theta_sorted) - 1
    return float(theta_sorted[idx])


def compute_angular_resolution(
    gamma_data: QTable, cuts: Optional[QTable] = None
) -> QTable:
    """
    Compute the 68% containment angular resolution based on gamma_data.

    Parameters
    ----------
    gamma_data : QTable
        Gamma events with columns: theta, true_energy, reco_energy, gh_score.
        Optionally includes 'weights' for weighted 68% containment.
    cuts : QTable, optional
        Cuts from optimize_cuts.py with: energy_low, energy_high, theta_cut, gh_cut.
        These cuts are applied based on reconstructed energy.

    Returns
    -------
    QTable
        energy_low, energy_high, angular_resolution_no_cuts, 
        angular_resolution_with_gh_cuts (if cuts provided), 
        angular_resolution_with_all_cuts (if cuts provided).
        *Always binned on true_energy*.
    """
    theta = gamma_data["theta"].to(u.deg).value
    true_energy_val = gamma_data["true_energy"].to(u.TeV).value
    
    # 修复权重读取逻辑
    if "weights" in gamma_data.colnames:
        weights = np.asarray(gamma_data["weights"])
    else:
        weights = np.ones(len(theta))

    # 统一使用 True Energy 进行分 Bin (这里沿用你之前的 16 个对数 Bin 的逻辑)
    energy_min = np.min(true_energy_val)
    energy_max = np.max(true_energy_val)
    true_energy_bins = np.logspace(np.log10(energy_min), np.log10(energy_max), 16)
    true_e_lows = true_energy_bins[:-1]
    true_e_lows = cuts["energy_low"].to(u.TeV).value
    true_e_highs = cuts["energy_high"].to(u.TeV).value

    if cuts is None:
        angular_resolutions = []
        for e_low, e_high in zip(true_e_lows, true_e_highs):
            mask = (true_energy_val >= e_low) & (true_energy_val < e_high)
            t = theta[mask]
            w = weights[mask] if weights is not None else None
            angular_resolutions.append(_percentile_68(t, w))

        return QTable({
            "energy_low": true_e_lows * u.TeV,
            "energy_high": true_e_highs * u.TeV,
            "angular_resolution": np.array(angular_resolutions) * u.deg,
        })

    # === 第一步：根据 Reco Energy 全局应用 Cuts，给每个事件打标签 ===
    reco_energy_val = gamma_data["reco_energy"].to(u.TeV).value
    gh_score = np.asarray(gamma_data["gh_score"])

    cut_e_lows = np.asarray(cuts["energy_low"].to(u.TeV).value)
    cut_e_highs = np.asarray(cuts["energy_high"].to(u.TeV).value)
    theta_cuts = np.asarray(cuts["theta_cut"].to(u.deg).value)
    gh_cuts = np.asarray(cuts["gh_cut"])

    # 初始化布尔类型的 Mask，默认为 False
    pass_gh = np.zeros(len(gamma_data), dtype=bool)
    pass_all = np.zeros(len(gamma_data), dtype=bool)

    for i in range(len(cuts)):
        # 找到属于当前 Reco Energy Bin 的事件
        reco_mask = (reco_energy_val >= cut_e_lows[i]) & (reco_energy_val < cut_e_highs[i])
        
        # 在这些事件中，哪些通过了 GH cut
        gh_mask_this_bin = reco_mask & (gh_score < gh_cuts[i])
        pass_gh |= gh_mask_this_bin # 更新全局 Mask
        
        # 在这些事件中，哪些同时通过了 GH cut 和 Theta cut
        pass_all |= gh_mask_this_bin & (theta < theta_cuts[i])

    # === 第二步：在 True Energy Bin 下计算角分辨 ===
    result_no_cuts = []
    result_gh_cuts = []
    result_all_cuts = []

    for e_low, e_high in zip(true_e_lows, true_e_highs):
        # 现在的区间划分完全依赖 True Energy
        true_mask = (true_energy_val >= e_low) & (true_energy_val < e_high)
        
        if not np.any(true_mask):
            result_no_cuts.append(np.nan)
            result_gh_cuts.append(np.nan)
            result_all_cuts.append(np.nan)
            continue

        # 当前 True Energy Bin 下的所有事件属性
        t_bin = theta[true_mask]
        w_bin = weights[true_mask]
        
        # 获取当前 True Energy Bin 内，事件是否通过了之前的全局筛选
        mask_gh_true = pass_gh[true_mask]
        mask_all_true = pass_all[true_mask]

        # 1. No cuts
        result_no_cuts.append(_percentile_68(t_bin, w_bin))

        # 2. GH cuts only
        if np.any(mask_gh_true) and np.sum(w_bin[mask_gh_true]) > 0:
            result_gh_cuts.append(_percentile_68(t_bin[mask_gh_true], w_bin[mask_gh_true]))
        else:
            result_gh_cuts.append(np.nan)

        # 3. All cuts
        if np.any(mask_all_true) and np.sum(w_bin[mask_all_true]) > 0:
            result_all_cuts.append(_percentile_68(t_bin[mask_all_true], w_bin[mask_all_true]))
        else:
            result_all_cuts.append(np.nan)

    return QTable({
        "energy_low": true_e_lows * u.TeV,
        "energy_high": true_e_highs * u.TeV,
        "angular_resolution_no_cuts": np.array(result_no_cuts) * u.deg,
        "angular_resolution_with_gh_cuts": np.array(result_gh_cuts) * u.deg,
        "angular_resolution_with_all_cuts": np.array(result_all_cuts) * u.deg,
    })
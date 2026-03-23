/**
 * @file histogram_helper.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief  helper function for histograms(Mostly based on TH2D)
 * @version 0.1
 * @date 2026-01-20
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include "TH2D.h"

struct InterpResult {
  double value;
  bool inside;   // 是否在直方图范围内
  bool bilinear; // 是否满足完整2x2（不在边界）
  bool is_valid() const { return inside; }
};

inline InterpResult interpolate_histogram(TH2D *histogram, double x, double y,
                                   double fill_value = 0);

InterpResult interpolate_histogram(TH2D *histogram, double x, double y,
                                   double fill_value) {
  InterpResult result;
  int bin_x = histogram->GetXaxis()->FindBin(x);
  int bin_y = histogram->GetYaxis()->FindBin(y);
  if (bin_x < 1 || bin_x > histogram->GetXaxis()->GetNbins() || bin_y < 1 ||
      bin_y > histogram->GetYaxis()->GetNbins()) {
    result.inside = false;
    result.bilinear = false;
    return result;
  }

  // Check whether the bin is filled by default value (Not valid data)
  double bin_value = histogram->GetBinContent(bin_x, bin_y);
  if (fabs(bin_value - fill_value) < 1e-6) {
    result.inside = false;
    result.bilinear = false;
    return result;
  }
  result.inside = true;
  result.value = bin_value;
  result.bilinear = false;
  double xc = histogram->GetXaxis()->GetBinCenter(bin_x);
  double yc = histogram->GetYaxis()->GetBinCenter(bin_y);

  // 确定 4 个包围点的索引 (Q11, Q12, Q21, Q22)
  // 如果 x < 中心，则利用左边的 Bin；否则利用右边的 Bin
  int x1_idx = (x < xc) ? bin_x - 1 : bin_x;
  int x2_idx = x1_idx + 1;
  int y1_idx = (y < yc) ? bin_y - 1 : bin_y;
  int y2_idx = y1_idx + 1;

  // 快速 Lambda：检查某个 Bin 是否可用 (不越界 且 不是 fill_value)
  auto isValidBin = [&](int bx, int by) {
    if (bx < 1 || bx > histogram->GetNbinsX() || by < 1 ||
        by > histogram->GetNbinsY())
      return false;
    double val = histogram->GetBinContent(bx, by);
    return std::abs(val - fill_value) > 1e-6;
  };

  // 必须 4 个点全部有效才能做双线性插值
  if (isValidBin(x1_idx, y1_idx) && isValidBin(x2_idx, y1_idx) &&
      isValidBin(x1_idx, y2_idx) && isValidBin(x2_idx, y2_idx)) {

    // 获取 4 个点的值
    double q11 = histogram->GetBinContent(x1_idx, y1_idx);
    double q21 = histogram->GetBinContent(x2_idx, y1_idx);
    double q12 = histogram->GetBinContent(x1_idx, y2_idx);
    double q22 = histogram->GetBinContent(x2_idx, y2_idx);

    // 获取坐标用于计算权重
    double x1 = histogram->GetXaxis()->GetBinCenter(x1_idx);
    double x2 = histogram->GetXaxis()->GetBinCenter(x2_idx);
    double y1 = histogram->GetYaxis()->GetBinCenter(y1_idx);
    double y2 = histogram->GetYaxis()->GetBinCenter(y2_idx);

    // 标准双线性插值公式
    double t = (x - x1) / (x2 - x1);
    double u = (y - y1) / (y2 - y1);

    double interp_val = (1 - t) * (1 - u) * q11 + t * (1 - u) * q21 +
                        (1 - t) * u * q12 + t * u * q22;

    result.value = interp_val;
    result.bilinear = true;
  }

  // 如果上面 if 不通过，result 保持为 z0 (最近邻)，且 valid=true
  return result;
}
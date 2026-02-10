#pragma once
#include "Eigen/Dense"
#include "Eigen/src/Core/Matrix.h"
#include "Eigen_extension/Eigen/LevenbergMarquardt"
namespace {

    // 定义 2D 高斯模型用于 Levenberg-Marquardt
    // 参数顺序 b: [0]Amp, [1]MeanX, [2]MeanY, [3]Length, [4]Width, [5]Psi
    struct Gaussian2DFunctor : Eigen::DenseFunctor<double> {
        const Eigen::VectorXd& x_map;
        const Eigen::VectorXd& y_map;
        const Eigen::VectorXd& image; // 实际观测到的光强 (masked_image)
    
        Gaussian2DFunctor(const Eigen::VectorXd& x, const Eigen::VectorXd& y, const Eigen::VectorXd& i)
            : Eigen::DenseFunctor<double>(6, x.size()), x_map(x), y_map(y), image(i) {}
    
        // 计算残差向量: fvec[i] = Observed[i] - Model[i]
        int operator()(const Eigen::VectorXd &b, Eigen::VectorXd &fvec) const {
            double A = b(0);
            double mx = b(1);
            double my = b(2);
            double sig_L = b(3); // Length (沿长轴的 sigma)
            double sig_W = b(4); // Width (沿短轴的 sigma)
            double psi = b(5);
    
            // 预计算旋转参数
            double cos_p = std::cos(psi);
            double sin_p = std::sin(psi);
            
            // 增加数值稳定性保护，防止除以 0
            // 使用平方形式可以避免 sqrt，同时加上 epsilon
            double var_L = sig_L * sig_L;
            double var_W = sig_W * sig_W;
            if (var_L < 1e-8) var_L = 1e-8;
            if (var_W < 1e-8) var_W = 1e-8;
    
            // 遍历每个像素计算残差
            for (int i = 0; i < values(); ++i) {
                double dx = x_map(i) - mx;
                double dy = y_map(i) - my;
    
                // 核心魔法：坐标旋转
                // 将像素坐标投影到簇射的主轴坐标系 (Longitudinal, Transverse)
                double x_prime = dx * cos_p + dy * sin_p;
                double y_prime = -dx * sin_p + dy * cos_p;
    
                // 计算标准高斯值 (在旋转后的坐标系中，没有 xy 交叉项)
                double exponent = -0.5 * ( (x_prime * x_prime) / var_L + (y_prime * y_prime) / var_W );
                
                // 限制指数范围防止溢出 (虽然 exp(-inf) = 0 通常很安全)
                double model_val = 0.0;
                if (exponent > -20) { // 只有在靠近中心时才计算，极远处理论上为0
                     model_val = A * std::exp(exponent);
                }
    
                fvec(i) = image(i) - model_val;
            }
            return 0;
        }
    };
    
    } // namespace
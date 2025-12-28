#include "TestReconstructor.hh"
#include "Eigen/Dense"
#include "spdlog/spdlog.h"
#include <unordered_map>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include "Minuit2/FCNBase.h"
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/MnMigrad.h"
#include "Minuit2/MnSimplex.h"
#include "Minuit2/FunctionMinimum.h"
#include "ReconstructorFactory.hh"


static bool registered_test_reconstructor = []() {
    ReconstructorFactory::instance().register_reconstructor("TestReconstructor", [](const SubarrayDescription& subarray, const json& config) -> std::unique_ptr<GeometryReconstructor> {
    return std::make_unique<TestReconstructor>(subarray, config);
});
    return true;
}();


template <typename Derived>
std::span<const float> as_span(const Eigen::MatrixBase<Derived>& m) {
    return { m.derived().data(),
             static_cast<std::size_t>(m.size()) };
}

// Functor class for Minuit2 minimization
class LikelihoodFCN : public ROOT::Minuit2::FCNBase {
public:
    LikelihoodFCN(TestReconstructor* reconstructor, ArrayEvent* event) 
        : reconstructor_(reconstructor), event_(event), errorDef_(0.5), iteration_(0) {}
    
    double operator()(const std::vector<double>& par) const override {
        // par[0] = rec_x
        // par[1] = rec_y
        // par[2] = rec_tilted_core_x
        // par[3] = rec_tilted_core_y
        // par[4] = xmax
        // par[5] = log10_energy
        double likelihood = reconstructor_->total_likelihood(par[0], par[1], par[2], par[3], par[4], par[5], *event_);
        
        // Log each iteration
        //spdlog::info("Minuit iteration {}: rec_x={:.6f}, rec_y={:.6f}, core_x={:.2f}, core_y={:.2f}, xmax={:.2f}, energy={:.2f}, -logL={:.4f}", 
        //            iteration_++, par[0], par[1], par[2], par[3], par[4], std::pow(10, par[5]), likelihood);
        
        return likelihood;
    }
    
    double Up() const override { return errorDef_; }
    
private:
    TestReconstructor* reconstructor_;
    ArrayEvent* event_;
    double errorDef_;
    mutable int iteration_;
};

void TestReconstructor::operator()(ArrayEvent& event)
{
    GeometryReconstructor::operator()(event);
    reconstructed_energy.energy_valid = false;
    if(!event.dl2->geometry["HillasReconstructor"].is_valid)
    {
        spdlog::warn("HillasReconstructor is not valid, skipping test reconstruction");
        geometry.is_valid = false;
        event.dl2->geometry[this->name()] = geometry;
        reconstructed_energy.energy_valid = false;
        event.dl2->add_energy(this->name() + "_energy", reconstructed_energy);
        return;
    }
    if(!event.dl2->energy.contains("EnergyRegressor") || !event.dl2->energy["EnergyRegressor"].energy_valid)
    {
        spdlog::warn("EnergyRegressor is not valid, skipping test reconstruction");
        geometry.is_valid = false;
        event.dl2->geometry[this->name()] = geometry;
        reconstructed_energy.energy_valid = false;
        event.dl2->add_energy(this->name() + "_energy", reconstructed_energy);
        return;
    }
    
    // Get initial values from HillasReconstructor
    auto [initial_x, initial_y] = convert_to_fov(event.dl2->geometry["HillasReconstructor"].alt, event.dl2->geometry["HillasReconstructor"].az);
    double initial_core_x = event.dl2->geometry["HillasReconstructor"].core_x;
    double initial_core_y = event.dl2->geometry["HillasReconstructor"].core_y;
    double tilted_core_x = event.dl2->geometry["HillasReconstructor"].tilted_core_x;
    double tilted_core_y = event.dl2->geometry["HillasReconstructor"].tilted_core_y;
    double xmax = event.dl2->geometry["HillasReconstructor"].xmax;
    double log10_energy = log10(event.dl2->energy["EnergyRegressor"].estimate_energy);

    double mean_xmax = (300 + 93 * log10_energy)/std::cos(20 * M_PI / 180 );
    double diff_xmax = xmax - mean_xmax;
    if(diff_xmax < -200)
        diff_xmax = -200;
    if(diff_xmax > 250)
        diff_xmax = 250;

    tilted_frame = std::make_unique<TiltedGroundFrame>(array_pointing_direction.azimuth, array_pointing_direction.altitude);
    tiled_tel_pos = get_tiled_tel_position(*tilted_frame);
    auto [true_x, true_y] = convert_to_fov(event.simulation->shower.alt, event.simulation->shower.az);
    double true_energy = event.simulation->shower.energy;
    double true_mean_xmax = (300 + 93 * std::log10(true_energy))/std::cos(20 * M_PI / 180 );
    double true_core_x = event.simulation->shower.core_x;
    double true_core_y = event.simulation->shower.core_y;
    double true_xmax = event.simulation->shower.x_max;

    double true_diff_xmax = true_xmax - true_mean_xmax;
    if(true_diff_xmax < -200)
        true_diff_xmax = -200;
    if(true_diff_xmax > 250)
        true_diff_xmax = 250;

    auto core_pos = CartesianPoint(true_core_x, true_core_y, 0);
    auto tilted_core_pos = core_pos.transform_to_tilted(*tilted_frame);
    double true_tilted_core_x = tilted_core_pos.x();
    double true_tilted_core_y = tilted_core_pos.y();
    
    spdlog::info("True values:    rec_x={:.6f}, rec_y={:.6f}, core_x={:.2f}, core_y={:.2f}, xmax={:.2f} energy={:.2f}", true_x, true_y, true_core_x, true_core_y, true_xmax, true_energy);
    spdlog::info("Initial values: rec_x={:.6f}, rec_y={:.6f}, core_x={:.2f}, core_y={:.2f}, xmax={:.2f} energy={:.2f}", 
                 initial_x, initial_y, tilted_core_x, tilted_core_y, xmax, std::pow(10, log10_energy));
    
    // Create FCN functor for Minuit2
    LikelihoodFCN fcn(this, &event);

    double likelihood_true_value = fcn(std::vector<double>{true_x, true_y, true_tilted_core_x, true_tilted_core_y, true_diff_xmax, std::log10(true_energy)});
    spdlog::info("Likelihood of true values: {:.4f}", likelihood_true_value);
    
    // Create MnUserParameters with initial values and step sizes
    ROOT::Minuit2::MnStrategy strategy(1);
    ROOT::Minuit2::MnUserParameters upar;
    upar.Add("rec_x", initial_x * 1, 0.01/57.3 * 1);              // step: 0.01 degrees
    upar.Add("rec_y", initial_y * 1, 0.01/57.3 * 1);              // step: 0.01 degrees
    upar.Add("tilted_core_x", tilted_core_x, 30);   // step: 1 meter
    upar.Add("tilted_core_y", tilted_core_y, 30);   // step: 1 meter
    upar.Add("xmax", diff_xmax, 60.0);                    // step: 30 g/cm²
    upar.Add("log10_energy", log10_energy, 0.1);      // step: 0.1
    
    // Set parameter limits (ranges)
    upar.SetLimits("rec_x", (initial_x - 0.5/57.3) * 1, (initial_x + 0.5/57.3) * 1);      // ±1 degree
    upar.SetLimits("rec_y", (initial_y - 0.5/57.3) * 1, (initial_y + 0.5/57.3) * 1);      // ±1 degrees
    upar.SetLimits("tilted_core_x", tilted_core_x - 50, tilted_core_x + 50);  // ±50 meters
    upar.SetLimits("tilted_core_y", tilted_core_y - 50, tilted_core_y + 50);  // ±50 meters
    upar.SetLimits("xmax",  -200, 250);                               // 200-1200 g/cm²
    upar.SetLimits("log10_energy", log10_energy - 0.2, log10_energy + 0.2);                           // 1-2

    upar.Fix("rec_x");
    upar.Fix("rec_y");
    upar.Fix("tilted_core_x");
    upar.Fix("tilted_core_y");

    spdlog::info("=== Step1: Running Simplex Minimization ===");
    ROOT::Minuit2::MnSimplex simplex(fcn, upar, strategy);
    ROOT::Minuit2::FunctionMinimum min_simplex = simplex(10000, 800);
    // 准备最终结果容器
    ROOT::Minuit2::MnUserParameters best_params = upar; // 默认回退到 Hillas (初始值)
    double best_fval = 1e9; // 一个很大的初始 Likelihood
    bool is_fit_valid = false;
    std::string method_used = "Hillas(Failed)";

    if(min_simplex.IsValid())
    {
        best_params = min_simplex.UserParameters();
        double simplex_xmax = best_params.Value("xmax");
        best_fval = min_simplex.Fval();
        is_fit_valid = true;
        method_used = "Simplex";
        spdlog::info("===Stpe2: Running MIGRAD STARTING FROM SIMPLEX MINIMIZATION ===");
        best_params.SetError("xmax", 35);
        best_params.SetLimits("xmax", simplex_xmax -80, simplex_xmax + 80);
        best_params.SetLimits("log10_energy", log10_energy - 0.2, log10_energy + 0.2);                           // 1-2
        best_params.Release("log10_energy");
        best_params.Release("rec_x");
        best_params.Release("rec_y");
        best_params.Release("tilted_core_x");
        best_params.Release("tilted_core_y");
        ROOT::Minuit2::MnMigrad migrad(fcn, best_params, strategy);
        ROOT::Minuit2::FunctionMinimum min_migrad = migrad(40000, 1000);
        if(min_migrad.IsValid())
        {
            best_params = min_migrad.UserParameters();
            best_fval = min_migrad.Fval();
            is_fit_valid = true;
            method_used = "MIGRAD";
        }
        else
        {
            spdlog::warn("MIGRAD minimization failed, use Simplex Minimization result");
            is_fit_valid = true;
        }
    }
    else
    {
        spdlog::warn("Simplex minimization failed, use initial values");
        is_fit_valid = false;
        method_used = "Hillas(Fallback)";
    }
    // Create minimizer (MIGRAD is the standard algorithm)
    spdlog::info("Reconstruction finished with method: {}", method_used);
    
    
    
    // Extract results
    double final_rec_x = best_params.Value("rec_x");
    double final_rec_y = best_params.Value("rec_y");
    double final_tilted_core_x = best_params.Value("tilted_core_x");
    double final_tilted_core_y = best_params.Value("tilted_core_y");
    double final_xmax = best_params.Value("xmax");
    double final_energy = std::pow(10, best_params.Value("log10_energy"));
    double final_mean_xmax = (300 + 93 * best_params.Value("log10_energy"))/std::cos(20 * M_PI / 180 );
    // Log final results
    spdlog::info("=== Minimization Completed Successfully ===");
    spdlog::info("Final values:   rec_x={:.6f}, rec_y={:.6f}, core_x={:.2f}, core_y={:.2f}, xmax={:.2f}, energy={:.2f}", 
                 final_rec_x, final_rec_y, final_tilted_core_x, final_tilted_core_y, final_mean_xmax, final_energy);
    spdlog::info("True values:    rec_x={:.6f}, rec_y={:.6f}", true_x, true_y);
    //spdlog::info("Delta from true: dx={:.6f}, dy={:.6f}", final_rec_x - true_x, final_rec_y - true_y);
    //spdlog::info("Final -logL = {:.4f}", min.Fval());
    
    // Convert back to alt/az
    auto [final_az, final_alt] = convert_to_sky(final_rec_x, final_rec_y);
    auto tilted_core_position = CartesianPoint(final_tilted_core_x, final_tilted_core_y, 0);
    auto intersection_position = tilted_core_position.transform_to_ground(*tilted_frame);
    auto [core_x, core_y] = project_to_ground(intersection_position, SkyDirection(AltAzFrame(), array_pointing_direction.azimuth, array_pointing_direction.altitude));
    // Store results in geometry
    geometry.alt = final_alt;
    geometry.az = final_az;
    geometry.core_x = core_x;
    geometry.core_y = core_y;
    geometry.tilted_core_x = final_tilted_core_x;
    geometry.tilted_core_y = final_tilted_core_y;
    geometry.is_valid = true;
    geometry.xmax = mean_xmax + final_xmax;
    geometry.direction_error = compute_angle_separation(final_az, final_alt, event.simulation->shower.az, event.simulation->shower.alt);
    geometry.telescopes = telescopes;
    
    event.dl2->geometry[this->name()] = geometry;
    reconstructed_energy.energy_valid = true;
    reconstructed_energy.estimate_energy = final_energy;
    reconstructed_energy.telescopes = telescopes;
    std::string energy_name = this->name() + "_energy";
    event.dl2->add_energy(energy_name, reconstructed_energy);
}

double TestReconstructor::get_log_likelihood( const Eigen::VectorXf& charge, const Eigen::Matrix<float, -1, 2, Eigen::RowMajor>& pix_pos, const float d, const float xmax, const float log10_energy)
{

    Eigen::Matrix<float, -1, -1, Eigen::RowMajor> phys;
    phys.resize(charge.size(), 3);
    phys.col(0) = Eigen::VectorXf::Constant(charge.size(), log10_energy);
    phys.col(1) = Eigen::VectorXf::Constant(charge.size(), xmax);
    phys.col(2) = Eigen::VectorXf::Constant(charge.size(), d);
    auto pix_prob = onnx_runner->PredictProb(as_span(charge), as_span(phys), as_span(pix_pos));
    double res = 0;

    for(int i = 0; i < pix_prob.size(); i++)
    {
        if(pix_prob[i] > 5)
        {
            res += std::log(5);
        }
        else
        {
            res += std::log(pix_prob[i]);
        }
    }
    return res;
}

double TestReconstructor::total_likelihood(double rec_x, double rec_y,
                                           double rec_tilted_core_x, double rec_tilted_core_y,
                                           double xmax, double log10_energy, ArrayEvent& event)
{
    rec_x = rec_x / 1;
    rec_y = rec_y / 1;
    double sum_loglikelihood = 0;

    for (const auto& telescope : telescopes)
    {
        double tel_x = tiled_tel_pos.at(telescope).x();
        double tel_y = tiled_tel_pos.at(telescope).y();

        double impact_parameter =
            std::sqrt(pow(tel_x - rec_tilted_core_x, 2) +
                      pow(tel_y - rec_tilted_core_y, 2));
        

        double psi = -std::atan2(rec_tilted_core_y - tel_y,
                                 rec_tilted_core_x - tel_x);

        const auto& image = event.simulation->tels.at(telescope)->fake_image;
        const auto& mask  = event.simulation->tels.at(telescope)->fake_image_mask;

        /// Number of True pixels
        const int nmask = mask.count();

        /// Prepare masked arrays
        Eigen::VectorXf scalecleaned_image(nmask);
        Eigen::Matrix<float, -1, 2, Eigen::RowMajor> rotated_pix_pos(nmask, 2);

        const auto  pix_x = subarray.tels.at(telescope).camera_description
                                .camera_geometry.pix_x
                            / subarray.tels.at(telescope).optics_description
                                .effective_focal_length;

        const auto  pix_y = subarray.tels.at(telescope).camera_description
                                .camera_geometry.pix_y
                            / subarray.tels.at(telescope).optics_description
                                .effective_focal_length;

        /// --- Fill arrays only where mask[i] == true ---
        int k = 0;
        for (int i = 0; i < pix_x.size(); i++)
        {
            if (mask[i])
            {
                /// log(charge)
                //scalecleaned_image[k] = std::pow(image[i], 1.0/3);
                scalecleaned_image[k] = std::log(image[i]);

                /// rotated pixel coordinates
                double dx = pix_x[i] - rec_x;
                double dy = pix_y[i] - rec_y;

                rotated_pix_pos(k, 0) =  (dx * std::cos(psi) - dy * std::sin(psi)) * 180 / M_PI;
                rotated_pix_pos(k, 1) = (dx * std::sin(psi) + dy * std::cos(psi)) * 180 / M_PI;
                ++k;
            }
        }

        /// Compute this telescope’s log-likelihood
        double likelihood =
            get_log_likelihood(scalecleaned_image, rotated_pix_pos,
                               impact_parameter, xmax, log10_energy);

        sum_loglikelihood += likelihood;
    }

    return -sum_loglikelihood * 2;  // minimize negative log-likelihood
}

// Functor for direction profiling - fixes rec_x, rec_y and optimizes core position and xmax
class DirectionProfileFCN : public ROOT::Minuit2::FCNBase {
public:
    DirectionProfileFCN(TestReconstructor* reconstructor, ArrayEvent* event, double fixed_rec_x, double fixed_rec_y) 
        : reconstructor_(reconstructor), event_(event), fixed_rec_x_(fixed_rec_x), fixed_rec_y_(fixed_rec_y), errorDef_(0.5) {}
    
    double operator()(const std::vector<double>& par) const override {
        // par[0] = tilted_core_x
        // par[1] = tilted_core_y
        // par[2] = xmax
        // par[3] = log10_energy
        return reconstructor_->total_likelihood(fixed_rec_x_, fixed_rec_y_, par[0], par[1], par[2], par[3], *event_);
    }
    
    double Up() const override { return errorDef_; }
    
private:
    TestReconstructor* reconstructor_;
    ArrayEvent* event_;
    double fixed_rec_x_;
    double fixed_rec_y_;
    double errorDef_;
};

// Profile likelihood over angular direction grid
#include <thread>
#include <atomic>
#include <vector>

Eigen::MatrixXf TestReconstructor::profile_direction(const Eigen::VectorXf& rec_x_array, 
                                                     const Eigen::VectorXf& rec_y_array, 
                                                     double initial_core_x, 
                                                     double initial_core_y, 
                                                     double initial_xmax, 
                                                     ArrayEvent& event)
{
    int n_x = rec_x_array.size();
    int n_y = rec_y_array.size();
    int total_tasks = n_x * n_y;

    // 初始化结果矩阵，默认填充一个表示失败的大数值
    Eigen::MatrixXf result(n_x, n_y);
    result.setConstant(1e9f);

    // 获取 Energy 初值
    double initial_log10_energy = 1.0; 
    if(event.dl2->energy.contains("EnergyRegressor") && event.dl2->energy["EnergyRegressor"].energy_valid) {
        initial_log10_energy = std::log10(event.dl2->energy["EnergyRegressor"].estimate_energy);
    }

    // 确定线程数量 (使用硬件并发数，如果获取失败则默认 4)
    unsigned int n_threads = std::thread::hardware_concurrency();
    if (n_threads == 0) n_threads = 4;
    
    spdlog::info("Starting Profile Scan: {}x{} grid using {} threads", n_x, n_y, n_threads);

    // 原子计数器，指向下一个待处理的任务索引
    std::atomic<int> task_counter{0};

    // Worker 函数：每个线程不断获取任务直到队列为空
    auto worker = [&](int thread_id) {
        // 每个线程拥有自己独立的 Minuit Strategy 实例
        ROOT::Minuit2::MnStrategy strategy(1); 

        // 循环获取任务
        while (true) {
            // 1. 获取任务索引 (原子操作)
            int task_idx = task_counter.fetch_add(1);
            if (task_idx >= total_tasks) {
                break; // 所有任务已完成
            }

            // 2. 解码索引为 (i, j)
            int i = task_idx / n_y;
            int j = task_idx % n_y;

            double curr_rec_x = (rec_x_array[i]);
            double curr_rec_y = (rec_y_array[j]);

            // 3. 设置 Minuit 参数
            // 注意：必须在循环内部创建 FCN 和 UserParameters，确保线程安全且状态独立
            LikelihoodFCN local_fcn(this, &event); 
            ROOT::Minuit2::MnUserParameters upar;

            // 固定 Grid 坐标
            upar.Add("rec_x", curr_rec_x, 0.0); 
            upar.Add("rec_y", curr_rec_y, 0.0);
            upar.Fix("rec_x");
            upar.Fix("rec_y");

            // 释放并设置 Nuisance Parameters
            upar.Add("tilted_core_x", initial_core_x, 30.0); 
            upar.Add("tilted_core_y", initial_core_y, 30.0);
            upar.Add("xmax", initial_xmax, 50.0);
            upar.Add("log10_energy", initial_log10_energy, 0.05);
            upar.SetLimits("tilted_core_x", initial_core_x - 100.0, initial_core_x + 100.0);
            upar.SetLimits("tilted_core_y", initial_core_y - 100.0, initial_core_y + 100.0);
            upar.SetLimits("xmax", -200, 250); 
            upar.SetLimits("log10_energy", initial_log10_energy - 0.15, initial_log10_energy + 0.15);

            // 4. 执行最小化 (Simplex)
            ROOT::Minuit2::MnMigrad simplex(local_fcn, upar, strategy);
            
            // 抑制 Minuit 内部打印 (可选，视 Minuit 版本而定，通常设置 print level)
            // simplex.SetPrintLevel(0); 

            // 限制迭代次数以保证速度 (500次通常足够收敛局部极值)
            ROOT::Minuit2::FunctionMinimum min = simplex(30000, 100.0);

            // 5. 写入结果
            // 由于不同线程处理不同的 (i, j)，且 MatrixXf 的内存是连续预分配的，
            // 只要不 resize，直接写入 result(i,j) 是线程安全的，不需要锁。
            if (min.IsValid()) {
                std::cout << "min.Fval()=" << min.Fval() << std::endl;
                result(i, j) = static_cast<float>(min.Fval());
            } else {
                std::cout << "Simplex minimization failed at rec_x=" << curr_rec_x << ", rec_y=" << curr_rec_y << std::endl;
                result(i, j) = 1e9f; // 标记失败
            }
        }
    };

    // 启动线程池
    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (unsigned int t = 0; t < n_threads; ++t) {
        threads.emplace_back(worker, t);
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    spdlog::info("Profile Scan Completed.");
    return result;
}
// Functor for core profiling - fixes core_x, core_y and optimizes direction and xmax
class CoreProfileFCN : public ROOT::Minuit2::FCNBase {
public:
    CoreProfileFCN(TestReconstructor* reconstructor, ArrayEvent* event, double fixed_core_x, double fixed_core_y) 
        : reconstructor_(reconstructor), event_(event), fixed_core_x_(fixed_core_x), fixed_core_y_(fixed_core_y), errorDef_(0.5) {}
    
    double operator()(const std::vector<double>& par) const override {
        // par[0] = rec_x
        // par[1] = rec_y
        // par[2] = xmax
        // par[3] = log10_energy
        return reconstructor_->total_likelihood(par[0], par[1], fixed_core_x_, fixed_core_y_, par[2], par[3], *event_);
    }
    
    double Up() const override { return errorDef_; }
    
private:
    TestReconstructor* reconstructor_;
    ArrayEvent* event_;
    double fixed_core_x_;
    double fixed_core_y_;
    double errorDef_;
};

// Profile likelihood over core position grid
Eigen::MatrixXf TestReconstructor::profile_core(const Eigen::VectorXf& core_x_array, 
                                                 const Eigen::VectorXf& core_y_array,
                                                 double initial_rec_x, 
                                                 double initial_rec_y,
                                                 double initial_xmax, 
                                                 ArrayEvent& event)
{
    const int nx = core_x_array.size();
    const int ny = core_y_array.size();
    const int total_points = nx * ny;
    
    // Result matrix: rows = core_x, cols = core_y
    Eigen::MatrixXf likelihood_map(nx, ny);
    
    spdlog::info("=== Profiling Core Position Likelihood (with optimization) ===");
    spdlog::info("Grid size: {} x {} points", nx, ny);
    spdlog::info("core_x range: [{:.2f}, {:.2f}]", core_x_array[0], core_x_array[nx-1]);
    spdlog::info("core_y range: [{:.2f}, {:.2f}]", core_y_array[0], core_y_array[ny-1]);
    spdlog::info("Optimizing: rec_x, rec_y, xmax at each point");
    
    // Determine number of threads
    const unsigned int num_threads = std::thread::hardware_concurrency();
    spdlog::info("Using {} threads for parallel computation", num_threads);
    
    // Atomic counter for task distribution
    std::atomic<int> next_task(0);
    std::atomic<int> completed_points(0);
    std::mutex log_mutex;
    
    // Worker function - each thread processes individual grid points
    auto worker = [&]() {
        // Each thread uses its own warm start parameters
        double thread_best_rec_x = initial_rec_x;
        double thread_best_rec_y = initial_rec_y;
        double thread_best_xmax = initial_xmax;
        
        while(true) {
            // Get next task atomically
            int task_id = next_task.fetch_add(1);
            if(task_id >= total_points) break;
            
            // Convert linear task_id to (i, j) coordinates
            int i = task_id / ny;
            int j = task_id % ny;
            
            double core_x = core_x_array[i];
            double core_y = core_y_array[j];
            
            // Create FCN with fixed core position
            CoreProfileFCN fcn(this, &event, core_x, core_y);
            
            // Setup parameters to optimize (direction and xmax)
            ROOT::Minuit2::MnUserParameters upar;
            upar.Add("rec_x", thread_best_rec_x * 1, 0.005/57.3 * 1);
            upar.Add("rec_y", thread_best_rec_y * 1, 0.005/57.3 * 1);
            upar.Add("xmax", thread_best_xmax, 20.0);
            
            upar.SetLimits("rec_x", (thread_best_rec_x - 1/57.3) * 1, (thread_best_rec_x + 1/57.3) * 1);
            upar.SetLimits("rec_y", (thread_best_rec_y - 1/57.3) * 1, (thread_best_rec_y + 1/57.3) * 1);
            upar.SetLimits("xmax", 300, 700);
            
            // Minimize
            ROOT::Minuit2::MnMigrad migrad(fcn, upar);
            ROOT::Minuit2::FunctionMinimum min = migrad(30000, 1000);
            
            if(min.IsValid()) {
                likelihood_map(i, j) = min.Fval();
                // Update warm start parameters for this thread
                thread_best_rec_x = min.UserParameters().Value("rec_x") / 1;
                thread_best_rec_y = min.UserParameters().Value("rec_y") / 1;
                thread_best_xmax = min.UserParameters().Value("xmax");
            } else {
                // If minimization fails, use a large penalty value
                likelihood_map(i, j) = 1e10;
                std::lock_guard<std::mutex> lock(log_mutex);
                spdlog::warn("Minimization failed at core_x={:.2f}, core_y={:.2f}", core_x, core_y);
            }
            
            // Update progress
            int completed = completed_points.fetch_add(1) + 1;
            if(completed % std::max(1, total_points / 10) == 0)
            {
                std::lock_guard<std::mutex> lock(log_mutex);
                spdlog::info("Core profile progress: {:.0f}%", 100.0 * completed / total_points);
            }
        }
    };
    
    // Create and launch threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    for(unsigned int t = 0; t < num_threads; t++)
    {
        threads.emplace_back(worker);
    }
    
    // Wait for all threads to complete
    for(auto& thread : threads)
    {
        thread.join();
    }
    
    // Find minimum (best fit)
    int min_i, min_j;
    float min_likelihood = likelihood_map.minCoeff(&min_i, &min_j);
    
    spdlog::info("Core profile complete. Best fit: core_x={:.2f}, core_y={:.2f}, -logL={:.4f}", 
                 core_x_array[min_i], core_y_array[min_j], min_likelihood);
    
    return likelihood_map;
}

#include "OnnxRuntime/onnxruntime_cxx_api.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <type_traits>

class PcfOnnxRunner {
public:
    /// 构造函数：加载 ONNX 模型
    /// model_path: pcf.onnx 路径
    /// phys_dim / pos_dim: 你模型里的物理维度和位置维度
    PcfOnnxRunner(const std::string& model_path,
                  int phys_dim,
                  int pos_dim)
        : env_(ORT_LOGGING_LEVEL_WARNING, "pcf_runner"),
          session_options_{},
          session_(nullptr),
          memory_info_(Ort::MemoryInfo::CreateCpu(
              OrtAllocatorType::OrtArenaAllocator, OrtMemTypeDefault)),
          phys_dim_(phys_dim),
          pos_dim_(pos_dim) {
        // 配置 SessionOptions（可以根据需要改）
        session_options_.SetIntraOpNumThreads(1);
        session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

        // 创建 Session（真正加载模型）
        session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(), session_options_);

        // 如果你在导出时就指定了名字，这里可以直接写死：
        input_name_charge_ = "pe";
        input_name_phys_   = "phys";
        input_name_pos_    = "pos";

        // 输出名：为了简单，直接拿第 0 个输出的名字
        Ort::AllocatedStringPtr out_name = session_->GetOutputNameAllocated(0, allocator_);
        output_name_ = out_name.get();          // 复制到 std::string

        // 如果你知道输出名，也可以直接写死：
        // output_name_ = "output";
    }

    // 默认析构就够了：RAII 会帮你释放 env / session / Ort::Value 的内部资源
    ~PcfOnnxRunner() = default;

    /// PredictProb (原始版本 - 保持向后兼容):
    ///   输入 batch 的 (charge, phys, pos)
    ///   - charge: 长度 = batch_size（每个样本 1 维电荷）
    ///   - phys:   长度 = batch_size * phys_dim_ (行优先存储的二维矩阵)
    ///   - pos:    长度 = batch_size * pos_dim_ (行优先存储的二维矩阵)
    /// 返回:
    ///   长度 = batch_size 的概率密度 p
    template<typename T>
    std::vector<typename std::conditional<std::is_same_v<T, float>, float, double>::type>
    PredictProb(const std::vector<T>& charge,
                const std::vector<T>& phys,
                const std::vector<T>& pos) {
        return PredictProb(
            std::span<const T>(charge),
            std::span<const T>(phys),
            std::span<const T>(pos));
    }

    // 兼容老接口
    std::vector<float> PredictProb(const std::vector<float>& charge,
                                   const std::vector<float>& phys,
                                   const std::vector<float>& pos) {
        return PredictProb<float>(charge, phys, pos);
    }

    /// PredictProb (span 版本 - 更通用):
    ///   接受任何连续容器（vector, array, C数组等）
    ///   输入数据布局：
    ///   - charge: [batch_size] 一维数组
    ///   - phys:   [batch_size * phys_dim_] 行优先二维数组
    ///             即 phys[i*phys_dim_ + j] 表示第 i 个样本的第 j 个物理特征
    ///   - pos:    [batch_size * pos_dim_] 行优先二维数组
    ///             即 pos[i*pos_dim_ + j] 表示第 i 个样本的第 j 个位置特征
    template<typename T>
    std::vector<typename std::conditional<std::is_same_v<T, float>, float, double>::type>
    PredictProb(std::span<const T> charge,
                std::span<const T> phys,
                std::span<const T> pos) {

        using OutT = typename std::conditional<std::is_same_v<T, float>, float, double>::type;
        constexpr ONNXTensorElementDataType onnx_type =
            std::is_same_v<T, float> ? ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT : ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE;

        // ==== 1. 检查输入维度 ====
        if (charge.empty()) {
            throw std::invalid_argument("charge 不能为空");
        }

        const size_t batch_size = charge.size();
        const size_t expected_phys_size = batch_size * static_cast<size_t>(phys_dim_);
        const size_t expected_pos_size = batch_size * static_cast<size_t>(pos_dim_);

        if (phys.size() != expected_phys_size) {
            throw std::invalid_argument(
                "phys 大小不匹配: 期望 [" + std::to_string(batch_size) +
                " × " + std::to_string(phys_dim_) + " = " +
                std::to_string(expected_phys_size) +
                "], 实际 " + std::to_string(phys.size())
            );
        }

        if (pos.size() != expected_pos_size) {
            throw std::invalid_argument(
                "pos 大小不匹配: 期望 [" + std::to_string(batch_size) +
                " × " + std::to_string(pos_dim_) + " = " +
                std::to_string(expected_pos_size) +
                "], 实际 " + std::to_string(pos.size())
            );
        }

        // ==== 2. 构造 shape（使用 array 避免动态分配）====
        const std::array<int64_t, 2> charge_shape = {
            static_cast<int64_t>(batch_size), 1
        };
        const std::array<int64_t, 2> phys_shape = {
            static_cast<int64_t>(batch_size), phys_dim_
        };
        const std::array<int64_t, 2> pos_shape = {
            static_cast<int64_t>(batch_size), pos_dim_
        };

        // ==== 3. 把数据包装成 Ort::Value (Tensor) ====
        Ort::Value charge_tensor = Ort::Value::CreateTensor<T>(
            memory_info_,
            const_cast<T*>(charge.data()),
            charge.size(),
            charge_shape.data(),
            charge_shape.size()
        );

        Ort::Value phys_tensor = Ort::Value::CreateTensor<T>(
            memory_info_,
            const_cast<T*>(phys.data()),
            phys.size(),
            phys_shape.data(),
            phys_shape.size()
        );

        Ort::Value pos_tensor = Ort::Value::CreateTensor<T>(
            memory_info_,
            const_cast<T*>(pos.data()),
            pos.size(),
            pos_shape.data(),
            pos_shape.size()
        );

        // ==== 4. 准备输入输出名称 ====
        const std::array<const char*, 3> input_names = {
            input_name_charge_.c_str(),
            input_name_phys_.c_str(),
            input_name_pos_.c_str()
        };
        const std::array<const char*, 1> output_names = {
            output_name_.c_str()
        };

        std::array<Ort::Value, 3> input_tensors = {
            std::move(charge_tensor),
            std::move(phys_tensor),
            std::move(pos_tensor)
        };

        // ==== 5. 执行推理 ====
        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_names.data(),
            output_names.size()
        );

        if (output_tensors.empty()) {
            throw std::runtime_error("模型没有返回任何输出");
        }

        // ==== 6. 解析输出 ====
        Ort::Value& out = output_tensors[0];
        const auto out_info = out.GetTensorTypeAndShapeInfo();

        ONNXTensorElementDataType out_type = out_info.GetElementType();
        const auto out_shape = out_info.GetShape();

        // 验证输出形状
        if (out_shape.size() != 2 || out_shape[0] != static_cast<int64_t>(batch_size)) {
            throw std::runtime_error(
                "输出形状错误: 期望 [" + std::to_string(batch_size) + ", 1或2], 实际 [" +
                std::to_string(out_shape[0]) + ", " +
                std::to_string(out_shape[1]) + "]"
            );
        }

        std::vector<OutT> probs;
        probs.reserve(batch_size);

        // ==== 7. 支持输出 float 或 double ====
        if constexpr(std::is_same_v<T, float>) {
            if (out_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                const float* out_data = out.GetTensorData<float>();
                if (out_shape[1] == 1) {
                    for (size_t i = 0; i < batch_size; ++i) {
                        probs.push_back(out_data[i]/(1 - out_data[i]));
                    }
                } else {
                    throw std::runtime_error(
                        "不支持的输出维度: [" + std::to_string(batch_size) + ", " +
                        std::to_string(out_shape[1]) + "], 期望第二维是 1");
                }
            } else if (out_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
                // 模型输出为 double，输入为 float，自动转型
                const double* out_data = out.GetTensorData<double>();
                if (out_shape[1] == 1) {
                    for (size_t i = 0; i < batch_size; ++i) {
                        probs.push_back(static_cast<float>(out_data[i]/(1 - out_data[i])));
                    }
                } else {
                    throw std::runtime_error(
                        "不支持的输出维度: [" + std::to_string(batch_size) + ", " +
                        std::to_string(out_shape[1]) + "], 期望第二维是 1");
                }
            } else {
                throw std::runtime_error("输出类型既不是 float 也不是 double");
            }
        } else { // T = double
            if (out_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
                const double* out_data = out.GetTensorData<double>();
                if (out_shape[1] == 1) {
                    for (size_t i = 0; i < batch_size; ++i) {
                        probs.push_back(out_data[i]/(1 - out_data[i]));
                    }
                } else {
                    throw std::runtime_error(
                        "不支持的输出维度: [" + std::to_string(batch_size) + ", " +
                        std::to_string(out_shape[1]) + "], 期望第二维是 1");
                }
            } else if (out_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                const float* out_data = out.GetTensorData<float>();
                if (out_shape[1] == 1) {
                    for (size_t i = 0; i < batch_size; ++i) {
                        probs.push_back(static_cast<double>(out_data[i]/(1 - out_data[i])));
                    }
                } else {
                    throw std::runtime_error(
                        "不支持的输出维度: [" + std::to_string(batch_size) + ", " +
                        std::to_string(out_shape[1]) + "], 期望第二维是 1");
                }
            } else {
                throw std::runtime_error("输出类型既不是 float 也不是 double");
            }
        }
        return probs;
    }

    // 兼容老接口
    std::vector<float> PredictProb(std::span<const float> charge,
                                   std::span<const float> phys,
                                   std::span<const float> pos) {
        return PredictProb<float>(charge, phys, pos);
    }

    /// PredictProbMatrix (矩阵视图版本 - 最清晰的语义):
    ///   明确表示输入是二维矩阵
    ///   - charge: [batch_size] 一维数组
    ///   - phys:   [batch_size, phys_dim_] 二维矩阵（行优先存储）
    ///   - pos:    [batch_size, pos_dim_] 二维矩阵（行优先存储）
    /// 
    /// 使用示例：
    ///   std::vector<float> phys_flat = {...};  // batch_size * phys_dim 个元素
    ///   size_t batch_size = 10;
    ///   auto probs = runner.PredictProbMatrix(
    ///       charge_vec,
    ///       phys_flat, batch_size, phys_dim,
    ///       pos_flat, batch_size, pos_dim
    ///   );
    template<typename T>
    std::vector<typename std::conditional<std::is_same_v<T, float>, float, double>::type>
    PredictProbMatrix(
        std::span<const T> charge,
        std::span<const T> phys_flat, size_t phys_rows, size_t phys_cols,
        std::span<const T> pos_flat, size_t pos_rows, size_t pos_cols) {

        using OutT = typename std::conditional<std::is_same_v<T, float>, float, double>::type;

        // 验证矩阵维度
        const size_t batch_size = charge.size();

        if (phys_rows != batch_size) {
            throw std::invalid_argument(
                "phys 矩阵行数 (" + std::to_string(phys_rows) +
                ") 必须等于 batch_size (" + std::to_string(batch_size) + ")"
            );
        }

        if (phys_cols != static_cast<size_t>(phys_dim_)) {
            throw std::invalid_argument(
                "phys 矩阵列数 (" + std::to_string(phys_cols) +
                ") 必须等于 phys_dim (" + std::to_string(phys_dim_) + ")"
            );
        }

        if (pos_rows != batch_size) {
            throw std::invalid_argument(
                "pos 矩阵行数 (" + std::to_string(pos_rows) +
                ") 必须等于 batch_size (" + std::to_string(batch_size) + ")"
            );
        }

        if (pos_cols != static_cast<size_t>(pos_dim_)) {
            throw std::invalid_argument(
                "pos 矩阵列数 (" + std::to_string(pos_cols) +
                ") 必须等于 pos_dim (" + std::to_string(pos_dim_) + ")"
            );
        }

        if (phys_flat.size() != phys_rows * phys_cols) {
            throw std::invalid_argument(
                "phys_flat 大小 (" + std::to_string(phys_flat.size()) +
                ") 不匹配矩阵维度 " + std::to_string(phys_rows) +
                " × " + std::to_string(phys_cols)
            );
        }

        if (pos_flat.size() != pos_rows * pos_cols) {
            throw std::invalid_argument(
                "pos_flat 大小 (" + std::to_string(pos_flat.size()) +
                ") 不匹配矩阵维度 " + std::to_string(pos_rows) +
                " × " + std::to_string(pos_cols)
            );
        }

        // 委托给 span 版本
        return PredictProb<T>(charge, phys_flat, pos_flat);
    }

    // 兼容老接口
    std::vector<float> PredictProbMatrix(
        std::span<const float> charge,
        std::span<const float> phys_flat, size_t phys_rows, size_t phys_cols,
        std::span<const float> pos_flat, size_t pos_rows, size_t pos_cols) {
        return PredictProbMatrix<float>(
            charge, phys_flat, phys_rows, phys_cols, pos_flat, pos_rows, pos_cols);
    }

private:
    // ==== 成员顺序很重要：Env 要在 Session 前面析构 ====
    Ort::Env env_;
    Ort::SessionOptions session_options_;
    std::unique_ptr<Ort::Session> session_;
    Ort::AllocatorWithDefaultOptions allocator_;
    Ort::MemoryInfo memory_info_;

    int phys_dim_;
    int pos_dim_;

    std::string input_name_charge_;
    std::string input_name_phys_;
    std::string input_name_pos_;
    std::string output_name_;
};
#include "LightGBM/c_api.h"


class LGBMPredictor {
public:
    LGBMPredictor(const std::string& model_path);
    ~LGBMPredictor();
    double predict(const std::vector<double>& features);
private:
    BoosterHandle booster_{nullptr};
    int num_total_model_{0};
};
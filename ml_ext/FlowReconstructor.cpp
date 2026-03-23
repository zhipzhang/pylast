#include "FlowReconstructor.hh"
#include "Eigen/Dense"
#include "Minuit2/FCNBase.h"
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/MnMigrad.h"
#include "Minuit2/FunctionMinimum.h"
#include "ReconstructorFactory.hh"



static bool registered_flow_reconstructor = []() {
    ReconstructorFactory::instance().register_creator("FlowReconstructor", [](const SubarrayDescription& subarray, const json& config) -> std::unique_ptr<GeometryReconstructor> {
    return std::make_unique<FlowReconstructor>(subarray, config);
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
    LikelihoodFCN(FlowReconstructor* reconstructor, ArrayEvent* event) 
        : reconstructor_(reconstructor), event_(event), errorDef_(0.5), iteration_(0) {}
    
    double operator()(const std::vector<double>& par) const override {
        // par[0] = rec_x
        // par[1] = rec_y
        // par[2] = rec_tilted_core_x
        // par[3] = rec_tilted_core_y
        // par[4] = xmax
        // par[5] = log10_energy
        double likelihood = reconstructor_->total_likelihood(par[0], par[1], par[2], par[3], par[4], par[5], *event_);
        return likelihood;
    }
    
    double Up() const override { return errorDef_; }
    
    private:
    FlowReconstructor* reconstructor_;
    ArrayEvent* event_;
    double errorDef_;
    mutable int iteration_;
};
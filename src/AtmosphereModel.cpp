#include "AtmosphereModel.hh"


TableAtmosphereModel::TableAtmosphereModel(int n_alt, double* alt_km, double* rho, double* thick, double* refidx_m1)
{
    if(n_alt <= 0 || alt_km == nullptr || rho == nullptr || thick == nullptr || refidx_m1 == nullptr) {
        throw std::runtime_error("n_alt must be greater than 0 and all input arrays must be non-null");
    }
    this->n_alt = n_alt;
    this->alt_km = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(alt_km, n_alt));
    this->rho = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(rho, n_alt));
    this->thick = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(thick, n_alt));
    this->refidx_m1 = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(refidx_m1, n_alt));
}


#include "AtmosphereModel.hh"


TableAtmosphereModel::TableAtmosphereModel(int n_alt, double* alt_km, double* rho, double* thick, double* refidx_m1)
{
    this->n_alt = n_alt;
    this->alt_km = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(alt_km, n_alt));
    this->rho = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(rho, n_alt));
    this->thick = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(thick, n_alt));
    this->refidx_m1 = Eigen::VectorXd(Eigen::Map<Eigen::VectorXd>(refidx_m1, n_alt));
}

TableAtmosphereModel& TableAtmosphereModel::operator=(TableAtmosphereModel&& other) noexcept
{
    this->n_alt = other.n_alt;
    this->alt_km = std::move(other.alt_km);
    this->rho = std::move(other.rho);
    this->thick = std::move(other.thick);
    this->refidx_m1 = std::move(other.refidx_m1);
    this->input_filename = other.input_filename;
    return *this;
}

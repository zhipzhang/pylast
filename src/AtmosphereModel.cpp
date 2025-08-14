#include "AtmosphereModel.hh"
#include <filesystem>
#include <fstream>

TableAtmosphereModel::TableAtmosphereModel(const std::string& filename) : input_filename(filename)
{
    if (!std::filesystem::exists(filename)) {
        throw std::runtime_error(fmt::format("Atmosphere model file '{}' does not exist.", filename));
    }
    // Load the atmosphere model data from the file
    std::ifstream file(input_filename);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Could not open atmosphere model file '{}'.", filename));
    }

    std::vector<double> alt_data, rho_data, thick_data, refidx_data;
    std::string line;

    // Skip header lines
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        double alt, rho, thick, refidx;
        
        if (iss >> alt >> rho >> thick >> refidx) {
            alt_data.push_back(alt);
            rho_data.push_back(rho);
            thick_data.push_back(thick);
            refidx_data.push_back(refidx);
        }
    }

    file.close();

    if (alt_data.empty()) {
        throw std::runtime_error("No valid data found in atmosphere model file.");
    }

    this->n_alt = alt_data.size();
    this->alt_km = Eigen::Map<Eigen::VectorXd>(alt_data.data(), n_alt);
    this->rho = Eigen::Map<Eigen::VectorXd>(rho_data.data(), n_alt);
    this->thick = Eigen::Map<Eigen::VectorXd>(thick_data.data(), n_alt);
    this->refidx_m1 = Eigen::Map<Eigen::VectorXd>(refidx_data.data(), n_alt);
}
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


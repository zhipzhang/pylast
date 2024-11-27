#pragma once
#include "Basebind.hh"
#include <format>
class SimulationConfig: public Basebind
{
    public:
        SimulationConfig() = default;
        ~SimulationConfig() = default;
        /** @brief Original sim_telarray run number */
        int run_number = -1;

        /** @brief CORSIKA version * 1000 */
        float corsika_version;

        /** @brief sim_telarray version * 1000 */
        float simtel_version;

        /** @brief Lower limit of energy range of primary particle (TeV) */
        float energy_range_min;

        /** @brief Upper limit of energy range of primary particle (TeV) */
        float energy_range_max;

        /** @brief Total geomagnetic field (uT) */
        float prod_site_B_total;

        /** @brief Magnetic declination (rad) */
        float prod_site_B_declination;

        /** @brief Magnetic inclination (rad) */
        float prod_site_B_inclination;

        /** @brief Height of observation level (m) */
        float prod_site_alt;

        /** @brief Power-law spectral index of spectrum */
        float spectral_index;

        /** @brief Time when shower simulation started, CORSIKA: only date */
        float shower_prog_start;

        /** @brief CORSIKA=1, ALTAI=2, KASCADE=3, MOCCA=4 */
        float shower_prog_id;

        /** @brief Time when detector simulation started */
        float detector_prog_start;

        /** @brief simtelarray=1 */
        float detector_prog_id;

        /** @brief Number of showers simulated */
        float n_showers;

        /** @brief Numbers of uses of each shower */
        float shower_reuse;

        /** @brief Maximum shower altitude (rad) */
        float max_alt;

        /** @brief Minimum shower altitude (rad) */
        float min_alt;

        /** @brief Maximum shower azimuth (rad) */
        float max_az;

        /** @brief Minimum shower azimuth (rad) */
        float min_az;

        /** @brief Diffuse Mode On/Off */
        bool diffuse = false;

        /** @brief Maximum viewcone radius (deg) */
        float max_viewcone_radius;

        /** @brief Minimum viewcone radius (deg) */
        float min_viewcone_radius;

        /** @brief Maximum scatter range (m) */
        float max_scatter_range;

        /** @brief Minimum scatter range (m) */
        float min_scatter_range;

        /** @brief Core Position Mode (0=Circular, 1=Rectangular) */
        float core_pos_mode;

        /** @brief Atmospheric model number */
        float atmosphere;

        /** @brief CORSIKA simulation options for IACTs */
        float corsika_iact_options;

        /** @brief CORSIKA low-energy simulation physics model */
        float corsika_low_E_model;

        /** @brief CORSIKA physics model ID for high energies (1=VENUS, 2=SIBYLL, 3=QGSJET, 4=DPMJET, 5=NeXus, 6=EPOS) */
        float corsika_high_E_model;

        /** @brief Number of Cherenkov photons per bunch */
        float corsika_bunchsize;

        /** @brief Minimum wavelength of cherenkov light (nm) */
        float corsika_wlen_min;

        /** @brief Maximum wavelength of cherenkov light (nm) */
        float corsika_wlen_max;

        /** @brief More details on low E interaction model (version etc.) */
        float corsika_low_E_detail;

        /** @brief More details on high E interaction model (version etc.) */
        float corsika_high_E_detail;

        /** @brief Prefix attached to column names when saved to a table or file */
        std::string prefix;

        void bind(nb::module_& m) override {
            nb::class_<SimulationConfig>(m, get_class_name().c_str())
                .def(nb::init<>())
                .def_ro("run_number", &SimulationConfig::run_number)
                .def_ro("corsika_version", &SimulationConfig::corsika_version)
                .def_ro("simtel_version", &SimulationConfig::simtel_version)
                .def_ro("energy_range_min", &SimulationConfig::energy_range_min)
                .def_ro("energy_range_max", &SimulationConfig::energy_range_max)
                .def_ro("prod_site_B_total", &SimulationConfig::prod_site_B_total)
                .def_ro("prod_site_B_declination", &SimulationConfig::prod_site_B_declination)
                .def_ro("prod_site_B_inclination", &SimulationConfig::prod_site_B_inclination)
                .def_ro("prod_site_alt", &SimulationConfig::prod_site_alt)
                .def_ro("spectral_index", &SimulationConfig::spectral_index)
                .def_ro("shower_prog_start", &SimulationConfig::shower_prog_start)
                .def_ro("shower_prog_id", &SimulationConfig::shower_prog_id)
                .def_ro("detector_prog_start", &SimulationConfig::detector_prog_start)
                .def_ro("detector_prog_id", &SimulationConfig::detector_prog_id)
                .def_ro("n_showers", &SimulationConfig::n_showers)
                .def_ro("shower_reuse", &SimulationConfig::shower_reuse)
                .def_ro("max_alt", &SimulationConfig::max_alt)
                .def_ro("min_alt", &SimulationConfig::min_alt)
                .def_ro("max_az", &SimulationConfig::max_az)
                .def_ro("min_az", &SimulationConfig::min_az)
                .def_ro("diffuse", &SimulationConfig::diffuse)
                .def_ro("max_viewcone_radius", &SimulationConfig::max_viewcone_radius)
                .def_ro("min_viewcone_radius", &SimulationConfig::min_viewcone_radius)
                .def_ro("max_scatter_range", &SimulationConfig::max_scatter_range)
                .def_ro("min_scatter_range", &SimulationConfig::min_scatter_range)
                .def_ro("core_pos_mode", &SimulationConfig::core_pos_mode)
                .def_ro("atmosphere", &SimulationConfig::atmosphere)
                .def_ro("corsika_iact_options", &SimulationConfig::corsika_iact_options)
                .def_ro("corsika_low_E_model", &SimulationConfig::corsika_low_E_model)
                .def_ro("corsika_high_E_model", &SimulationConfig::corsika_high_E_model)
                .def_ro("corsika_bunchsize", &SimulationConfig::corsika_bunchsize)
                .def_ro("corsika_wlen_min", &SimulationConfig::corsika_wlen_min)
                .def_ro("corsika_wlen_max", &SimulationConfig::corsika_wlen_max)
                .def_ro("corsika_low_E_detail", &SimulationConfig::corsika_low_E_detail)
                .def_ro("corsika_high_E_detail", &SimulationConfig::corsika_high_E_detail)
                .def_ro("prefix", &SimulationConfig::prefix);
        }

        const std::string print() const override {
            return std::format(
                "SimulationConfig:\n"
                "  Run Number: {}\n"
                "  Corsika Version: {}\n"
                "  Simtel Version: {}\n"
                "  Energy Range: {:.2f} - {:.2f} TeV\n"
                "  Production Site:\n"
                "    B Total: {:.2f} μT\n" 
                "    B Declination: {:.2f}°\n"
                "    B Inclination: {:.2f}°\n"
                "    Altitude: {:.2f} m\n"
                "  Spectral Index: {:.2f}\n"
                "  Shower Program: {} (ID: {})\n"
                "  Detector Program: {} (ID: {})\n"
                "  Showers: {} (reuse: {})\n"
                "  Pointing:\n"
                "    Alt Range: {:.2f}° - {:.2f}°\n"
                "    Az Range: {:.2f}° - {:.2f}°\n"
                "  Diffuse: {}\n"
                "  Viewcone Radius: {:.2f}° - {:.2f}°\n"
                "  Scatter Range: {:.2f} - {:.2f} m\n"
                "  Core Position Mode: {}\n"
                "  Atmosphere: {}\n"
                "  Corsika Options:\n"
                "    IACT Options: {}\n"
                "    Low E Model: {} ({})\n"
                "    High E Model: {} ({})\n"
                "    Bunch Size: {}\n"
                "    Wavelength Range: {:.2f} - {:.2f} nm\n",
                run_number,
                corsika_version,
                simtel_version,
                energy_range_min,
                energy_range_max,
                prod_site_B_total,
                prod_site_B_declination,
                prod_site_B_inclination,
                prod_site_alt,
                spectral_index,
                shower_prog_start,
                shower_prog_id,
                detector_prog_start,
                detector_prog_id,
                n_showers,
                shower_reuse,
                min_alt,
                max_alt,
                min_az,
                max_az,
                diffuse,
                min_viewcone_radius,
                max_viewcone_radius,
                min_scatter_range,
                max_scatter_range,
                core_pos_mode,
                atmosphere,
                corsika_iact_options,
                corsika_low_E_model,
                corsika_low_E_detail,
                corsika_high_E_model,
                corsika_high_E_detail,
                corsika_bunchsize,
                corsika_wlen_min,
                corsika_wlen_max
            );
            // Implementation of print method
        }

    protected:
        std::string get_class_name() const override {
            return "SimulationConfig";
        }

        

};
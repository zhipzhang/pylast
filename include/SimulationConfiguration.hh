/**
 * @file SimulationConfiguration.hh
 * @author Zach Peng
 * @brief Configuration parameters for the simulation (CORSIKA and simtelarray)
 * @version 0.1
 * @date 2024-11-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <string>
using std::string;

/**
 * @class SimulationConfiguration
 * @brief Configuration parameters for the simulation
 */
class SimulationConfiguration {
public:
    /// @brief Original sim_telarray run number
    int run_number = -1;

    /// @brief CORSIKA version * 1000
    float corsika_version = 0;

    /// @brief sim_telarray version * 1000
    float simtel_version = 0;
    
    /// @brief Lower limit of energy range of primary particle (TeV)
    float energy_range_min = 0;

    /// @brief Upper limit of energy range of primary particle (TeV)
    float energy_range_max = 0;
    
    /// @brief Total geomagnetic field (uT)
    float prod_site_B_total = 0;

    /// @brief Magnetic declination (rad)
    float prod_site_B_declination = 0;

    /// @brief Magnetic inclination (rad)
    float prod_site_B_inclination = 0;

    /// @brief Height of observation level (m)
    float prod_site_alt = 0;
    
    /// @brief Power-law spectral index of spectrum
    float spectral_index = 0;
    
    /// @brief Time when shower simulation started, CORSIKA: only date
    float shower_prog_start = 0;

    /// @brief CORSIKA=1, ALTAI=2, KASCADE=3, MOCCA=4
    float shower_prog_id = 0;

    /// @brief Time when detector simulation started
    float detector_prog_start = 0;

    /// @brief simtelarray=1
    float detector_prog_id = 0;
    
    /// @brief Number of showers simulated
    float n_showers = 0;

    /// @brief Numbers of uses of each shower
    float shower_reuse = 0;
    
    /// @brief Maximum shower altitude (rad)
    float max_alt = 0;

    /// @brief Minimum shower altitude (rad)
    float min_alt = 0;

    /// @brief Maximum shower azimuth (rad)
    float max_az = 0;

    /// @brief Minimum shower azimuth (rad)
    float min_az = 0;
    
    /// @brief Diffuse Mode On/Off
    bool diffuse = false;
    
    /// @brief Maximum viewcone radius (deg)
    float max_viewcone_radius = 0;

    /// @brief Minimum viewcone radius (deg)
    float min_viewcone_radius = 0;
    
    /// @brief Maximum scatter range (m)
    float max_scatter_range = 0;

    /// @brief Minimum scatter range (m)
    float min_scatter_range = 0;
    
    /// @brief Core Position Mode (0=Circular, 1=Rectangular)
    float core_pos_mode = 0;

    /// @brief Atmospheric model number
    float atmosphere = 0;
    
    /// @brief CORSIKA simulation options for IACTs
    float corsika_iact_options = 0;

    /// @brief CORSIKA low-energy simulation physics model
    float corsika_low_E_model = 0;

    /// @brief CORSIKA physics model ID for high energies (1=VENUS, 2=SIBYLL, 3=QGSJET, 4=DPMJET, 5=NeXus, 6=EPOS)
    float corsika_high_E_model = 0;

    /// @brief Number of Cherenkov photons per bunch
    float corsika_bunchsize = 0;

    /// @brief Minimum wavelength of cherenkov light (nm)
    float corsika_wlen_min = 0;

    /// @brief Maximum wavelength of cherenkov light (nm)
    float corsika_wlen_max = 0;

    /// @brief More details on low E interaction model (version etc.)
    float corsika_low_E_detail = 0;

    /// @brief More details on high E interaction model (version etc.)
    float corsika_high_E_detail = 0;

    const std::string print() const;
};


#include "LactEventSource.hh"

#include "DL0Event.hh"
#include "R1Event.hh"
#include "Pointing.hh"
#include "SimulatedCamera.hh"
#include "SimulatedEvent.hh"
#include "SimulatedShower.hh"
#include "SimulationConfiguration.hh"
#include "spdlog/spdlog.h"

#include "TTree.h"

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

template <typename T>
void set_branch_if_exists(TTree* tree, const char* name, T* address)
{
    if (tree != nullptr && tree->GetBranch(name) != nullptr) {
        tree->SetBranchAddress(name, address);
    }
}

TTree* required_tree(TFile* file, const char* name)
{
    auto* tree = static_cast<TTree*>(file->Get(name));
    if (tree == nullptr) {
        throw std::runtime_error(std::string("missing required LACT ROOT tree: ") + name);
    }
    return tree;
}

int lact_shape_to_pylast(int shape_code)
{
    // LACT: 1 square, 2 hexagonal, 3 circular.
    // pylast CameraGeometry: 0 circle, 1 hexagon, 2 square.
    if (shape_code == 1) return 2;
    if (shape_code == 2) return 1;
    if (shape_code == 3) return 0;
    return 1;
}

double pixel_area(double size_m, int shape_code)
{
    if (shape_code == 1) {
        return size_m * size_m;
    }
    if (shape_code == 2) {
        return 0.5 * std::sqrt(3.0) * size_m * size_m;
    }
    if (shape_code == 3) {
        return M_PI * size_m * size_m;
    }
    return size_m * size_m;
}

double deg_to_rad(double deg)
{
    return deg * M_PI / 180.0;
}

} // namespace

LactEventSource::LactEventSource(const std::string& filename,
                                 int64_t max_events,
                                 std::vector<int> subarray,
                                 bool load_simulated_showers)
    : EventSource(filename, max_events, subarray, load_simulated_showers)
{
    is_stream = false;
    initialize();
    load_schema();
    load_camera_pixels();
    load_optics();
    load_waveforms();
    load_telescopes();
    load_corsika_events();
    if (load_simulated_showers) {
        load_all_simulated_showers();
    }
    load_observations();
    build_event_order();
}

LactEventSource::~LactEventSource()
{
    if (file) {
        file->Close();
    }
}

void LactEventSource::open_file()
{
    file = std::unique_ptr<TFile>(TFile::Open(input_filename.c_str(), "READ"));
    if (!file || file->IsZombie()) {
        throw std::runtime_error("failed to open LACT ROOT file: " + input_filename);
    }
}

void LactEventSource::init_metaparam()
{
    spdlog::debug("LACT ROOT adapter does not currently populate metaparam");
}

void LactEventSource::init_atmosphere_model()
{
    spdlog::debug("LACT ROOT adapter does not currently populate atmosphere_model");
}

void LactEventSource::init_simulation_config()
{
    simulation_config = SimulationConfiguration();
}

void LactEventSource::init_subarray()
{
    subarray = SubarrayDescription();
}

void LactEventSource::load_all_simulated_showers()
{
    shower_array = SimulatedShowerArray(corsika_by_event.size());
    for (const auto& kv : corsika_by_event) {
        const auto& truth = kv.second;
        SimulatedShower shower;
        shower.energy = truth.energy_gev / 1000.0;
        shower.alt = deg_to_rad(truth.altitude_deg);
        shower.az = deg_to_rad(truth.azimuth_north_to_east_deg);
        shower.core_x = truth.core_x_north_m;
        shower.core_y = truth.core_y_west_m;
        shower.h_first_int = truth.h_first_int_m;
        shower.x_max = truth.x_max_g_cm2;
        shower.h_max = truth.h_max_m;
        shower.starting_grammage = truth.starting_grammage_g_cm2;
        shower.shower_primary_id = truth.primary_type;
        shower_array->push_back(shower);
    }
}

void LactEventSource::load_schema()
{
    auto* tree = required_tree(file.get(), "config");
    std::string* schema_name_ptr = nullptr;
    std::string* profile_ptr = nullptr;
    int schema_version_value = 0;
    int run_id_value = 0;
    set_branch_if_exists(tree, "schema_name", &schema_name_ptr);
    set_branch_if_exists(tree, "schema_version", &schema_version_value);
    set_branch_if_exists(tree, "profile", &profile_ptr);
    set_branch_if_exists(tree, "run_id", &run_id_value);
    tree->GetEntry(0);
    schema_name = schema_name_ptr ? *schema_name_ptr : "";
    schema_version = schema_version_value;
    profile = profile_ptr ? *profile_ptr : "";
    run_id = run_id_value;
    if (simulation_config.has_value()) {
        simulation_config->run_number = run_id;
    }
    if (schema_name != "lact_event_root" || schema_version != 1) {
        throw std::runtime_error("unsupported LACT ROOT schema: " +
                                 schema_name + " v" + std::to_string(schema_version));
    }
}

void LactEventSource::load_camera_pixels()
{
    auto* tree = required_tree(file.get(), "camera_pixels");
    CameraPixelRow row;
    set_branch_if_exists(tree, "pixel_id", &row.pixel_id);
    set_branch_if_exists(tree, "x_m", &row.x_m);
    set_branch_if_exists(tree, "y_m", &row.y_m);
    set_branch_if_exists(tree, "size_m", &row.size_m);
    set_branch_if_exists(tree, "shape_code", &row.shape_code);
    const auto n_entries = tree->GetEntries();
    camera_pixels.reserve(static_cast<std::size_t>(n_entries));
    for (Long64_t i = 0; i < n_entries; ++i) {
        tree->GetEntry(i);
        pixel_id_to_index[row.pixel_id] = static_cast<int>(camera_pixels.size());
        camera_pixels.push_back(row);
    }
}

void LactEventSource::load_optics()
{
    auto* tree = static_cast<TTree*>(file->Get("optics"));
    if (tree == nullptr || tree->GetEntries() == 0) {
        return;
    }
    std::string* name = nullptr;
    set_branch_if_exists(tree, "name", &name);
    set_branch_if_exists(tree, "num_mirrors", &optics.num_mirrors);
    set_branch_if_exists(tree, "mirror_area_m2", &optics.mirror_area_m2);
    set_branch_if_exists(tree, "equivalent_focal_length_m", &optics.equivalent_focal_length_m);
    set_branch_if_exists(tree, "effective_focal_length_m", &optics.effective_focal_length_m);
    tree->GetEntry(0);
    if (name != nullptr) {
        optics.name = *name;
    }
}

void LactEventSource::load_telescopes()
{
    auto* tree = required_tree(file.get(), "telescopes");
    TelescopeRow row;
    std::string* name = nullptr;
    set_branch_if_exists(tree, "telescope_id", &row.telescope_id);
    set_branch_if_exists(tree, "name", &name);
    set_branch_if_exists(tree, "array_x_north_m", &row.position[0]);
    set_branch_if_exists(tree, "array_y_west_m", &row.position[1]);
    set_branch_if_exists(tree, "array_z_up_m", &row.position[2]);
    set_branch_if_exists(tree, "pointing_az_deg", &row.pointing_az_deg);
    set_branch_if_exists(tree, "pointing_el_deg", &row.pointing_el_deg);
    const auto n_entries = tree->GetEntries();
    telescopes.reserve(static_cast<std::size_t>(n_entries));
    for (Long64_t i = 0; i < n_entries; ++i) {
        tree->GetEntry(i);
        if (name != nullptr) {
            row.name = *name;
        }
        telescopes.push_back(row);
    }

    if (!subarray.has_value()) {
        subarray = SubarrayDescription();
    }
    Eigen::VectorXd pix_x(camera_pixels.size());
    Eigen::VectorXd pix_y(camera_pixels.size());
    Eigen::VectorXd pix_area_vec(camera_pixels.size());
    Eigen::VectorXi pix_type(camera_pixels.size());
    for (std::size_t i = 0; i < camera_pixels.size(); ++i) {
        const auto& pix = camera_pixels[i];
        pix_x[static_cast<int>(i)] = pix.x_m;
        pix_y[static_cast<int>(i)] = pix.y_m;
        pix_area_vec[static_cast<int>(i)] = pixel_area(pix.size_m, pix.shape_code);
        pix_type[static_cast<int>(i)] = lact_shape_to_pylast(pix.shape_code);
    }
    CameraGeometry geometry("LACT", static_cast<int>(camera_pixels.size()),
                            pix_x, pix_y, pix_area_vec, pix_type, 0.0);
    geometry.pix_id.resize(static_cast<int>(camera_pixels.size()));
    for (std::size_t i = 0; i < camera_pixels.size(); ++i) {
        geometry.pix_id[static_cast<int>(i)] = camera_pixels[i].pixel_id;
    }
    const double focal_length = std::isfinite(optics.effective_focal_length_m) &&
            optics.effective_focal_length_m > 0.0
        ? optics.effective_focal_length_m
        : optics.equivalent_focal_length_m;
    if (std::isfinite(focal_length) && focal_length > 0.0) {
        geometry.pix_x_fov = geometry.pix_x / focal_length;
        geometry.pix_y_fov = geometry.pix_y / focal_length;
        geometry.pix_width_fov = geometry.pix_width / focal_length;
    }

    CameraReadout readout;
    readout.camera_name = "LACT";
    readout.n_pixels = static_cast<int>(camera_pixels.size());
    readout.n_samples = waveform_config.available ? waveform_config.n_time_bins : 1;
    readout.n_channels = 1;
    readout.reference_pulse_sample_width = waveform_config.time_bin_width_ns;
    readout.sampling_rate = waveform_config.time_bin_width_ns > 0.0
        ? 1.0e9 / waveform_config.time_bin_width_ns
        : 0.0;

    for (const auto& tel : telescopes) {
        if (!keep_tel(tel.telescope_id)) {
            continue;
        }
        TelescopeDescription description;
        description.tel_name = tel.name;
        description.camera_description.camera_name = "LACT";
        description.camera_description.camera_geometry = geometry;
        description.camera_description.camera_readout = readout;
        description.optics_description.optics_name = optics.name;
        description.optics_description.num_mirrors = optics.num_mirrors;
        description.optics_description.mirror_area = optics.mirror_area_m2;
        description.optics_description.equivalent_focal_length = optics.equivalent_focal_length_m;
        description.optics_description.effective_focal_length = optics.effective_focal_length_m;
        subarray->add_telescope(tel.telescope_id, std::move(description), tel.position);
    }
}

void LactEventSource::load_corsika_events()
{
    auto* tree = static_cast<TTree*>(file->Get("corsika_events"));
    if (tree == nullptr) {
        return;
    }
    CorsikaEventRow row;
    set_branch_if_exists(tree, "event_id", &row.event_id);
    set_branch_if_exists(tree, "shower_event_id", &row.shower_event_id);
    set_branch_if_exists(tree, "array_id", &row.array_id);
    set_branch_if_exists(tree, "run_id", &row.run_id);
    set_branch_if_exists(tree, "primary_type", &row.primary_type);
    set_branch_if_exists(tree, "energy_gev", &row.energy_gev);
    set_branch_if_exists(tree, "altitude_deg", &row.altitude_deg);
    set_branch_if_exists(tree, "azimuth_north_to_east_deg", &row.azimuth_north_to_east_deg);
    set_branch_if_exists(tree, "core_x_north_m", &row.core_x_north_m);
    set_branch_if_exists(tree, "core_y_west_m", &row.core_y_west_m);
    set_branch_if_exists(tree, "h_first_int_m", &row.h_first_int_m);
    set_branch_if_exists(tree, "x_max_g_cm2", &row.x_max_g_cm2);
    set_branch_if_exists(tree, "h_max_m", &row.h_max_m);
    set_branch_if_exists(tree, "starting_grammage_g_cm2", &row.starting_grammage_g_cm2);
    const auto n_entries = tree->GetEntries();
    for (Long64_t i = 0; i < n_entries; ++i) {
        tree->GetEntry(i);
        corsika_by_event[row.event_id] = row;
    }
}

void LactEventSource::load_observations()
{
    auto* tree = required_tree(file.get(), "observations");
    ObservationRow row;
    std::vector<int>* pixel_id = nullptr;
    std::vector<float>* image_pe = nullptr;
    std::vector<float>* peak_time = nullptr;
    set_branch_if_exists(tree, "event_id", &row.event_id);
    set_branch_if_exists(tree, "telescope_id", &row.telescope_id);
    set_branch_if_exists(tree, "triggered", &row.triggered);
    set_branch_if_exists(tree, "n_pixels_camera", &row.n_pixels_camera);
    set_branch_if_exists(tree, "pixel_id", &pixel_id);
    set_branch_if_exists(tree, "image_pe", &image_pe);
    set_branch_if_exists(tree, "image_time_peak_ns", &peak_time);
    const auto n_entries = tree->GetEntries();
    observations.reserve(static_cast<std::size_t>(n_entries));
    for (Long64_t i = 0; i < n_entries; ++i) {
        tree->GetEntry(i);
        if (!keep_tel(row.telescope_id)) {
            continue;
        }
        row.pixel_id = pixel_id ? *pixel_id : std::vector<int>{};
        row.image_pe = image_pe ? *image_pe : std::vector<float>{};
        row.image_time_peak_ns = peak_time ? *peak_time : std::vector<float>{};
        observation_index[{row.event_id, row.telescope_id}] = observations.size();
        observations.push_back(row);
    }
}

void LactEventSource::load_waveforms()
{
    auto* cfg_tree = static_cast<TTree*>(file->Get("waveform_config"));
    if (cfg_tree != nullptr && cfg_tree->GetEntries() > 0) {
        std::vector<double>* time_centers = nullptr;
        set_branch_if_exists(cfg_tree, "n_time_bins", &waveform_config.n_time_bins);
        set_branch_if_exists(cfg_tree, "time_bin_width_ns", &waveform_config.time_bin_width_ns);
        set_branch_if_exists(cfg_tree, "time_centers_ns", &time_centers);
        cfg_tree->GetEntry(0);
        waveform_config.available = true;
        if (time_centers != nullptr) {
            waveform_config.time_centers_ns = *time_centers;
        }
    }

    auto* tree = static_cast<TTree*>(file->Get("waveforms"));
    if (tree == nullptr) {
        return;
    }
    WaveformRow row;
    std::vector<int>* pixel_id = nullptr;
    std::vector<unsigned short>* time_bin = nullptr;
    std::vector<float>* pe = nullptr;
    set_branch_if_exists(tree, "event_id", &row.event_id);
    set_branch_if_exists(tree, "telescope_id", &row.telescope_id);
    set_branch_if_exists(tree, "n_pixels_camera", &row.n_pixels_camera);
    set_branch_if_exists(tree, "n_time_bins", &row.n_time_bins);
    set_branch_if_exists(tree, "pixel_id", &pixel_id);
    set_branch_if_exists(tree, "time_bin", &time_bin);
    set_branch_if_exists(tree, "pe", &pe);
    const auto n_entries = tree->GetEntries();
    for (Long64_t i = 0; i < n_entries; ++i) {
        tree->GetEntry(i);
        if (!keep_tel(row.telescope_id)) {
            continue;
        }
        row.pixel_id = pixel_id ? *pixel_id : std::vector<int>{};
        row.time_bin = time_bin ? *time_bin : std::vector<unsigned short>{};
        row.pe = pe ? *pe : std::vector<float>{};
        waveforms[{row.event_id, row.telescope_id}] = row;
        if (row.n_time_bins > 0) {
            waveform_config.available = true;
            waveform_config.n_time_bins = row.n_time_bins;
        }
    }
}

void LactEventSource::build_event_order()
{
    std::set<long long> ordered;
    for (const auto& obs : observations) {
        ordered.insert(obs.event_id);
    }
    event_order.assign(ordered.begin(), ordered.end());
    if (max_events == -1 || max_events > static_cast<int64_t>(event_order.size())) {
        max_events = static_cast<int64_t>(event_order.size());
    }
}

bool LactEventSource::keep_tel(int tel_id) const
{
    return allowed_tels.empty() ||
        std::find(allowed_tels.begin(), allowed_tels.end(), tel_id) != allowed_tels.end();
}

int LactEventSource::pixel_index(int pixel_id) const
{
    const auto it = pixel_id_to_index.find(pixel_id);
    if (it == pixel_id_to_index.end()) {
        return -1;
    }
    return it->second;
}

Eigen::VectorXd LactEventSource::dense_image(const ObservationRow& obs) const
{
    Eigen::VectorXd image = Eigen::VectorXd::Zero(camera_pixels.size());
    const auto n = std::min(obs.pixel_id.size(), obs.image_pe.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int idx = pixel_index(obs.pixel_id[i]);
        if (idx >= 0) {
            image[idx] = static_cast<double>(obs.image_pe[i]);
        }
    }
    return image;
}

Eigen::VectorXd LactEventSource::dense_peak_time(const ObservationRow& obs) const
{
    Eigen::VectorXd peak_time =
        Eigen::VectorXd::Constant(camera_pixels.size(), std::numeric_limits<double>::quiet_NaN());
    const auto n = std::min(obs.pixel_id.size(), obs.image_time_peak_ns.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int idx = pixel_index(obs.pixel_id[i]);
        if (idx >= 0) {
            peak_time[idx] = static_cast<double>(obs.image_time_peak_ns[i]);
        }
    }
    return peak_time;
}

Eigen::Matrix<double, -1, -1, Eigen::RowMajor>
LactEventSource::dense_waveform(const ObservationRow& obs) const
{
    const auto wf_it = waveforms.find({obs.event_id, obs.telescope_id});
    const int n_samples = waveform_config.available
        ? std::max(1, waveform_config.n_time_bins)
        : 1;
    Eigen::Matrix<double, -1, -1, Eigen::RowMajor> waveform =
        Eigen::Matrix<double, -1, -1, Eigen::RowMajor>::Zero(camera_pixels.size(), n_samples);
    if (wf_it == waveforms.end()) {
        waveform.col(0) = dense_image(obs);
        return waveform;
    }

    const auto& wf = wf_it->second;
    const auto n = std::min({wf.pixel_id.size(), wf.time_bin.size(), wf.pe.size()});
    for (std::size_t i = 0; i < n; ++i) {
        const int pix = pixel_index(wf.pixel_id[i]);
        const int bin = static_cast<int>(wf.time_bin[i]);
        if (pix >= 0 && bin >= 0 && bin < waveform.cols()) {
            waveform(pix, bin) += static_cast<double>(wf.pe[i]);
        }
    }
    return waveform;
}

ArrayEvent LactEventSource::get_event()
{
    if (is_finished()) {
        return ArrayEvent();
    }
    return get_event(current_event_index);
}

ArrayEvent LactEventSource::get_event(int index)
{
    if (index < 0 || index >= max_events) {
        throw std::out_of_range("Index out of range: " + std::to_string(index));
    }
    const long long event_id = event_order[static_cast<std::size_t>(index)];

    ArrayEvent event;
    event.event_id = static_cast<int>(event_id);
    event.run_id = run_id;
    event.simulation = SimulatedEvent();
    event.r1 = R1Event();
    event.dl0 = DL0Event();
    event.simulation->shower.energy = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.alt = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.az = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.core_x = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.core_y = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.h_first_int = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.x_max = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.h_max = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.starting_grammage = std::numeric_limits<double>::quiet_NaN();
    event.simulation->shower.shower_primary_id = 0;
    event.pointing = Pointing();
    bool have_array_pointing = false;
    double array_pointing_az = 0.0;
    double array_pointing_alt = 0.0;
    for (const auto& tel : telescopes) {
        if (!keep_tel(tel.telescope_id)) {
            continue;
        }
        const double az = deg_to_rad(tel.pointing_az_deg);
        const double alt = deg_to_rad(tel.pointing_el_deg);
        event.pointing->add_tel(tel.telescope_id, PointingTelescope(az, alt));
        if (!have_array_pointing) {
            array_pointing_az = az;
            array_pointing_alt = alt;
            have_array_pointing = true;
        }
    }
    if (have_array_pointing) {
        event.pointing->set_array_pointing(array_pointing_az, array_pointing_alt);
    }

    const auto truth_it = corsika_by_event.find(event_id);
    if (truth_it != corsika_by_event.end()) {
        const auto& truth = truth_it->second;
        event.run_id = truth.run_id;
        event.simulation->shower.energy = truth.energy_gev / 1000.0;
        event.simulation->shower.alt = deg_to_rad(truth.altitude_deg);
        event.simulation->shower.az = deg_to_rad(truth.azimuth_north_to_east_deg);
        event.simulation->shower.core_x = truth.core_x_north_m;
        event.simulation->shower.core_y = truth.core_y_west_m;
        event.simulation->shower.h_first_int = truth.h_first_int_m;
        event.simulation->shower.x_max = truth.x_max_g_cm2;
        event.simulation->shower.h_max = truth.h_max_m;
        event.simulation->shower.starting_grammage = truth.starting_grammage_g_cm2;
        event.simulation->shower.shower_primary_id = truth.primary_type;
    }

    for (const auto& obs : observations) {
        if (obs.event_id != event_id || !keep_tel(obs.telescope_id)) {
            continue;
        }
        const Eigen::VectorXd image = dense_image(obs);
        const Eigen::VectorXd peak_time = dense_peak_time(obs);
        Eigen::Matrix<double, -1, -1, Eigen::RowMajor> waveform = dense_waveform(obs);
        Eigen::VectorXi gain_selection = Eigen::VectorXi::Zero(waveform.rows());
        event.r1->add_tel(obs.telescope_id,
                          R1Camera{static_cast<int>(waveform.rows()),
                                   static_cast<int>(waveform.cols()),
                                   std::move(waveform),
                                   std::move(gain_selection)});
        event.dl0->add_tel(obs.telescope_id,
                           DL0Camera{.image = image, .peak_time = peak_time});
        SimulatedCamera sim_camera;
        sim_camera.fake_image = image;
        sim_camera.pe_amplitude = image;
        sim_camera.pe_time = peak_time;
        sim_camera.true_image_sum = static_cast<int>(std::lround(image.sum()));
        sim_camera.true_image = image.cast<int>();
        sim_camera.impact_parameter = std::numeric_limits<double>::quiet_NaN();
        event.simulation->add_tel(obs.telescope_id, std::move(sim_camera));
        if (obs.triggered) {
            event.simulation->triggered_tels.push_back(obs.telescope_id);
        }
    }

    return event;
}

bool LactEventSource::is_finished()
{
    return current_event_index >= max_events;
}

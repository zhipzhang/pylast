/**
 * @file AkArrayBuilder.hh
 * @author Zach Peng (zhipzhang@mail.ustc.edu.cn)
 * @brief Awkward LayoutBuilder wrappers for filling an ArrayEvent into a
 *        single unified nested ak.Array.
 *
 *  Data layout overview
 *  --------------------
 *  events (N)
 *    event_id, run_id
 *    mjd
 *    simulation: option<{
 *      energy/alt/az/core_x/y/h_first_int/x_max/h_max/starting_grammage/shower_primary_id
 *      triggered_tels : list<int32>
 *      tels : {
 *        ids, true_image_sum, impact_parameter, time_range_10_90
 *        hillas, leakage, concentration, morphology, intensity, extra
 *        (same ImageParameters sub-records as DL1)
 *        (per-pixel images true_image/fake_image excluded — handle in C++)
 *      }
 *    }>
 *    pointing : option<{ array_azimuth/altitude, tel_ids/azimuth/altitude }>
 *    dl0 : option<{ tel_ids, image, peak_time }>
 *    dl1 : option<{
 *      tel_ids
 *      hillas        : { length/width/psi/x/y/skewness/kurtosis/intensity/r/phi/scale_ratio }
 *      leakage       : { pixels_width_1/2, intensity_width_1/2 }
 *      concentration : { cog, core, pixel }
 *      morphology    : { n_pixels, n_islands, n_small/medium/large_islands }
 *      intensity     : { max, mean, std, skewness, kurtosis }
 *      extra         : { miss, disp, theta, true_psi, cog_err, beta_err }
 *      (image, peak_time, mask excluded — O(n_pixels), handle in C++)
 *    }>
 *    dl2 : option<{
 *      geometry : {
 *        methods(list<str>), is_valid, alt, az, core_x/y, hmax, xmax, tel_ids
 *      }
 *      energy : {
 *        methods(list<str>), energy_valid, estimate, estimate_std, tel_ids
 *      }
 *      particle : {
 *        methods(list<str>), is_valid, hadroness, mrsl, mrsw, tel_ids
 *      }
 *      tels : {
 *        ids, estimate_energy, estimate_hadroness, estimate_disp,
 *        impact_distance[n_tels][n_geom_methods], impact_distance_error
 *      }
 *    }>
 *
 * @version 0.3
 * @date 2026-03-23
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "awkward/LayoutBuilder.h"
#include "ArrayEvent.hh"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace ak_builder_detail {

// ---------------------------------------------------------------------------
// Common template aliases
// ---------------------------------------------------------------------------
using UserDefinedMap = std::map<std::size_t, std::string>;

template <class... BUILDERS>
using RecordBuilder = awkward::LayoutBuilder::Record<UserDefinedMap, BUILDERS...>;

template <std::size_t field_name, class BUILDER>
using Field = awkward::LayoutBuilder::Field<field_name, BUILDER>;

template <class PRIMITIVE, class BUILDER>
using ListOffsetBuilder = awkward::LayoutBuilder::ListOffset<PRIMITIVE, BUILDER>;

template <class PRIMITIVE>
using NumpyBuilder = awkward::LayoutBuilder::Numpy<PRIMITIVE>;

template <typename PRIMITIVE, typename BUILDER>
using IndexedOptionBuilder = awkward::LayoutBuilder::IndexedOption<PRIMITIVE, BUILDER>;

// Shorthand aliases used across multiple records
using CharBuilder         = NumpyBuilder<uint8_t>;
using SingleStringBuilder = ListOffsetBuilder<int32_t, CharBuilder>;
using StringListBuilder   = ListOffsetBuilder<int64_t, SingleStringBuilder>;

using Int8ListBuilder       = ListOffsetBuilder<int64_t, NumpyBuilder<int8_t>>;
using IntListBuilder        = ListOffsetBuilder<int64_t, NumpyBuilder<int32_t>>;
using DoubleListBuilder     = ListOffsetBuilder<int64_t, NumpyBuilder<double>>;
using IntListListBuilder    = ListOffsetBuilder<int64_t, ListOffsetBuilder<int64_t, NumpyBuilder<int32_t>>>;
using DoubleListListBuilder = ListOffsetBuilder<int64_t, ListOffsetBuilder<int64_t, NumpyBuilder<double>>>;

// ============================================================
// Pointing sub-record
// ============================================================
enum PtField : std::size_t {
    pt_array_azimuth = 0,
    pt_array_altitude,
    pt_tel_ids,
    pt_tel_azimuth,
    pt_tel_altitude
};
using PtRecord = RecordBuilder<
    Field<pt_array_azimuth,  NumpyBuilder<double>>,
    Field<pt_array_altitude, NumpyBuilder<double>>,
    Field<pt_tel_ids,     ListOffsetBuilder<int64_t, NumpyBuilder<int32_t>>>,
    Field<pt_tel_azimuth, ListOffsetBuilder<int64_t, NumpyBuilder<double>>>,
    Field<pt_tel_altitude, ListOffsetBuilder<int64_t, NumpyBuilder<double>>>
>;

// ============================================================
// DL0 sub-record
// ============================================================
enum DL0Field : std::size_t {
    dl0_tel_ids = 0,
    dl0_image,
    dl0_peak_time
};
using DL0Record = RecordBuilder<
    Field<dl0_tel_ids,   ListOffsetBuilder<int64_t, NumpyBuilder<int32_t>>>,
    Field<dl0_image,     ListOffsetBuilder<int64_t, ListOffsetBuilder<int64_t, NumpyBuilder<double>>>>,
    Field<dl0_peak_time, ListOffsetBuilder<int64_t, ListOffsetBuilder<int64_t, NumpyBuilder<double>>>>
>;

// ============================================================
// Reusable per-telescope ImageParameters sub-records
// Each field is list<T> — one value per telescope per event.
// Used identically in both DL1Record and SimTelsRecord.
// (image/peak_time/mask are O(n_pixels) and excluded intentionally)
// ============================================================

// --- HillasParameter ---
enum HillasField : std::size_t {
    hil_length = 0, hil_width, hil_psi,
    hil_x, hil_y,
    hil_skewness, hil_kurtosis, hil_intensity,
    hil_r, hil_phi, hil_scale_ratio
};
using HillasRecord = RecordBuilder<
    Field<hil_length,      DoubleListBuilder>,
    Field<hil_width,       DoubleListBuilder>,
    Field<hil_psi,         DoubleListBuilder>,
    Field<hil_x,           DoubleListBuilder>,
    Field<hil_y,           DoubleListBuilder>,
    Field<hil_skewness,    DoubleListBuilder>,
    Field<hil_kurtosis,    DoubleListBuilder>,
    Field<hil_intensity,   DoubleListBuilder>,
    Field<hil_r,           DoubleListBuilder>,
    Field<hil_phi,         DoubleListBuilder>,
    Field<hil_scale_ratio, DoubleListBuilder>
>;

// --- LeakageParameter ---
enum LeakageField : std::size_t {
    leak_pixels_w1 = 0, leak_pixels_w2,
    leak_intensity_w1, leak_intensity_w2
};
using LeakageRecord = RecordBuilder<
    Field<leak_pixels_w1,    DoubleListBuilder>,
    Field<leak_pixels_w2,    DoubleListBuilder>,
    Field<leak_intensity_w1, DoubleListBuilder>,
    Field<leak_intensity_w2, DoubleListBuilder>
>;

// --- ConcentrationParameter ---
enum ConcField : std::size_t {
    conc_cog = 0, conc_core, conc_pixel,
    conc_frac2
};
using ConcentrationRecord = RecordBuilder<
    Field<conc_cog,   DoubleListBuilder>,
    Field<conc_core,  DoubleListBuilder>,
    Field<conc_pixel, DoubleListBuilder>,
    Field<conc_frac2, DoubleListBuilder>
>;

// --- MorphologyParameter  (integer counts) ---
enum MorphField : std::size_t {
    morph_n_pixels = 0, morph_n_islands,
    morph_n_small,      morph_n_medium, morph_n_large
};
using MorphologyRecord = RecordBuilder<
    Field<morph_n_pixels,  IntListBuilder>,
    Field<morph_n_islands, IntListBuilder>,
    Field<morph_n_small,   IntListBuilder>,
    Field<morph_n_medium,  IntListBuilder>,
    Field<morph_n_large,   IntListBuilder>
>;

// --- IntensityParameter ---
enum IntensField : std::size_t {
    intens_max = 0, intens_mean, intens_std,
    intens_skewness, intens_kurtosis
};
using IntensityRecord = RecordBuilder<
    Field<intens_max,      DoubleListBuilder>,
    Field<intens_mean,     DoubleListBuilder>,
    Field<intens_std,      DoubleListBuilder>,
    Field<intens_skewness, DoubleListBuilder>,
    Field<intens_kurtosis, DoubleListBuilder>
>;

// --- ExtraParameters ---
enum ExtraField : std::size_t {
    extra_miss = 0, extra_disp, extra_theta,
    extra_true_psi, extra_cog_err, extra_beta_err
};
using ExtraRecord = RecordBuilder<
    Field<extra_miss,     DoubleListBuilder>,
    Field<extra_disp,     DoubleListBuilder>,
    Field<extra_theta,    DoubleListBuilder>,
    Field<extra_true_psi, DoubleListBuilder>,
    Field<extra_cog_err,  DoubleListBuilder>,
    Field<extra_beta_err, DoubleListBuilder>
>;

// ============================================================
// DL1 sub-record  — Hillas + full ImageParameters, no pixel arrays
// (image, peak_time, mask are O(n_pixels) and excluded intentionally)
// ============================================================
enum DL1Field : std::size_t {
    dl1_tel_ids = 0,
    dl1_hillas,
    dl1_leakage,
    dl1_concentration,
    dl1_morphology,
    dl1_intensity,
    dl1_extra
};
using DL1Record = RecordBuilder<
    Field<dl1_tel_ids,       ListOffsetBuilder<int64_t, NumpyBuilder<int32_t>>>,
    Field<dl1_hillas,        HillasRecord>,
    Field<dl1_leakage,       LeakageRecord>,
    Field<dl1_concentration, ConcentrationRecord>,
    Field<dl1_morphology,    MorphologyRecord>,
    Field<dl1_intensity,     IntensityRecord>,
    Field<dl1_extra,         ExtraRecord>
>;

// ============================================================
// DL2 — geometry sub-record
// ============================================================
enum GeomField : std::size_t {
    geom_methods = 0,
    geom_is_valid,
    geom_alt,
    geom_alt_uncertainty,
    geom_az,
    geom_az_uncertainty,
    geom_direction_error,
    geom_core_x,
    geom_core_y,
    geom_core_pos_error,
    geom_hmax,
    geom_xmax,
    geom_tel_ids         // list<list<int32>>  telescopes per method
};
using GeometryRecord = RecordBuilder<
    Field<geom_methods,          StringListBuilder>,
    Field<geom_is_valid,         Int8ListBuilder>,
    Field<geom_alt,              DoubleListBuilder>,
    Field<geom_alt_uncertainty,  DoubleListBuilder>,
    Field<geom_az,               DoubleListBuilder>,
    Field<geom_az_uncertainty,   DoubleListBuilder>,
    Field<geom_direction_error,  DoubleListBuilder>,
    Field<geom_core_x,           DoubleListBuilder>,
    Field<geom_core_y,           DoubleListBuilder>,
    Field<geom_core_pos_error,   DoubleListBuilder>,
    Field<geom_hmax,             DoubleListBuilder>,
    Field<geom_xmax,             DoubleListBuilder>,
    Field<geom_tel_ids,          IntListListBuilder>
>;

// ============================================================
// DL2 — energy sub-record
// ============================================================
enum EnergyField : std::size_t {
    energy_methods = 0,
    energy_valid,
    energy_estimate,
    energy_estimate_std,
    energy_tel_ids
};
using EnergyRecord = RecordBuilder<
    Field<energy_methods,      StringListBuilder>,
    Field<energy_valid,        Int8ListBuilder>,
    Field<energy_estimate,     DoubleListBuilder>,
    Field<energy_estimate_std, DoubleListBuilder>,
    Field<energy_tel_ids,      IntListListBuilder>
>;

// ============================================================
// DL2 — particle sub-record
// ============================================================
enum PartField : std::size_t {
    part_methods = 0,
    part_valid,
    part_hadroness,
    part_mrsl,
    part_mrsw,
    part_tel_ids
};
using ParticleRecord = RecordBuilder<
    Field<part_methods,   StringListBuilder>,
    Field<part_valid,     Int8ListBuilder>,
    Field<part_hadroness, DoubleListBuilder>,
    Field<part_mrsl,      DoubleListBuilder>,
    Field<part_mrsw,      DoubleListBuilder>,
    Field<part_tel_ids,   IntListListBuilder>
>;

// ============================================================
// DL2 — per-telescope sub-record
// ============================================================
enum DL2TelField : std::size_t {
    dl2tel_ids = 0,
    dl2tel_estimate_energy,
    dl2tel_estimate_hadroness,
    dl2tel_estimate_disp,
    dl2tel_impact_distance,       // list<list<double>>  [n_tels][n_geom_methods]
    dl2tel_impact_distance_error
};
using DL2TelsRecord = RecordBuilder<
    Field<dl2tel_ids,                   IntListBuilder>,
    Field<dl2tel_estimate_energy,       DoubleListBuilder>,
    Field<dl2tel_estimate_hadroness,    DoubleListBuilder>,
    Field<dl2tel_estimate_disp,         DoubleListBuilder>,
    Field<dl2tel_impact_distance,       DoubleListListBuilder>,
    Field<dl2tel_impact_distance_error, DoubleListListBuilder>
>;

// ============================================================
// DL2 top-level sub-record  (contains the 4 nested records)
// ============================================================
enum DL2Field : std::size_t {
    dl2_geometry = 0,
    dl2_energy,
    dl2_particle,
    dl2_tels
};
using DL2Record = RecordBuilder<
    Field<dl2_geometry, GeometryRecord>,
    Field<dl2_energy,   EnergyRecord>,
    Field<dl2_particle, ParticleRecord>,
    Field<dl2_tels,     DL2TelsRecord>
>;

// ============================================================
// Simulation — per-telescope sub-record  (SimulatedCamera)
// Per-pixel images (true_image, fake_image) are intentionally excluded.
// ImageParameters sub-records are shared with DL1.
// ============================================================
enum SimTelField : std::size_t {
    simtel_ids = 0,
    simtel_true_image_sum,
    simtel_impact_parameter,
    simtel_time_range_10_90,
    simtel_hillas,
    simtel_leakage,
    simtel_concentration,
    simtel_morphology,
    simtel_intensity,
    simtel_extra
};
using SimTelsRecord = RecordBuilder<
    Field<simtel_ids,              IntListBuilder>,
    Field<simtel_true_image_sum,   IntListBuilder>,
    Field<simtel_impact_parameter, DoubleListBuilder>,
    Field<simtel_time_range_10_90, DoubleListBuilder>,
    Field<simtel_hillas,           HillasRecord>,
    Field<simtel_leakage,          LeakageRecord>,
    Field<simtel_concentration,    ConcentrationRecord>,
    Field<simtel_morphology,       MorphologyRecord>,
    Field<simtel_intensity,        IntensityRecord>,
    Field<simtel_extra,            ExtraRecord>
>;

// ============================================================
// Simulation sub-record
// ============================================================
enum SimField : std::size_t {
    sim_energy = 0,
    sim_alt,
    sim_az,
    sim_core_x,
    sim_core_y,
    sim_h_first_int,
    sim_x_max,
    sim_h_max,
    sim_starting_grammage,
    sim_primary_id,
    sim_triggered_tels,   // list<int32>
    sim_tels              // SimTelsRecord  (SimulatedCamera per telescope)
};
using SimRecord = RecordBuilder<
    Field<sim_energy,            NumpyBuilder<double>>,
    Field<sim_alt,               NumpyBuilder<double>>,
    Field<sim_az,                NumpyBuilder<double>>,
    Field<sim_core_x,            NumpyBuilder<double>>,
    Field<sim_core_y,            NumpyBuilder<double>>,
    Field<sim_h_first_int,       NumpyBuilder<double>>,
    Field<sim_x_max,             NumpyBuilder<double>>,
    Field<sim_h_max,             NumpyBuilder<double>>,
    Field<sim_starting_grammage, NumpyBuilder<double>>,
    Field<sim_primary_id,        NumpyBuilder<int32_t>>,
    Field<sim_triggered_tels,    ListOffsetBuilder<int64_t, NumpyBuilder<int32_t>>>,
    Field<sim_tels,              SimTelsRecord>
>;

// ============================================================
// Top-level record
// ============================================================
enum TopField : std::size_t {
    ev_event_id = 0,
    ev_run_id,
    ev_mjd,
    ev_simulation,
    ev_pointing,
    ev_dl0,
    ev_dl1,
    ev_dl2
};
using TopRecordBuilder = RecordBuilder<
    Field<ev_event_id,   NumpyBuilder<int32_t>>,
    Field<ev_run_id,     NumpyBuilder<int32_t>>,
    Field<ev_mjd,        NumpyBuilder<double>>,
    Field<ev_simulation, IndexedOptionBuilder<int64_t, SimRecord>>,
    Field<ev_pointing,   IndexedOptionBuilder<int64_t, PtRecord>>,
    Field<ev_dl0,        IndexedOptionBuilder<int64_t, DL0Record>>,
    Field<ev_dl1,        IndexedOptionBuilder<int64_t, DL1Record>>,
    Field<ev_dl2,        IndexedOptionBuilder<int64_t, DL2Record>>
>;

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

/// Append a vector of strings into a StringListBuilder (one list per call).
inline void append_string_list(StringListBuilder& slb,
                                const std::vector<std::string>& strs)
{
    slb.begin_list();
    for (const auto& s : strs) {
        auto& chars = slb.content().begin_list();
        for (unsigned char c : s) chars.append(c);
        slb.content().end_list();
    }
    slb.end_list();
}

/// Return sorted keys of any associative container.
template <typename Map>
inline std::vector<std::string> sorted_keys(const Map& m)
{
    std::vector<std::string> keys;
    keys.reserve(m.size());
    for (const auto& [k, _] : m) keys.push_back(k);
    std::sort(keys.begin(), keys.end());
    return keys;
}

/// Mark a StringListBuilder's char buffer with awkward string/char parameters.
/// set_parameters takes the *content* of the JSON parameters object (no outer {}).
inline void mark_string_builder(StringListBuilder& slb)
{
    slb.content().set_parameters("\"__array__\": \"string\"");
    slb.content().content().set_parameters("\"__array__\": \"char\"");
}

// ---------------------------------------------------------------------------
// Fill helpers
// ---------------------------------------------------------------------------

inline void fill_dl0(DL0Record& rec, const DL0Event& dl0)
{
    auto& tel_ids_b = rec.content<dl0_tel_ids>();
    auto& image_b   = rec.content<dl0_image>();
    auto& pktime_b  = rec.content<dl0_peak_time>();

    tel_ids_b.begin_list();
    image_b.begin_list();
    pktime_b.begin_list();

    for (int tel_id : dl0.get_ordered_tels()) {
        const DL0Camera* cam = dl0.get_tel(tel_id);
        if (!cam) continue;
        tel_ids_b.content().append(static_cast<int32_t>(tel_id));

        auto& img_inner = image_b.content().begin_list();
        for (Eigen::Index i = 0; i < cam->image.size(); ++i)
            img_inner.append(cam->image[i]);
        image_b.content().end_list();

        auto& pk_inner = pktime_b.content().begin_list();
        for (Eigen::Index i = 0; i < cam->peak_time.size(); ++i)
            pk_inner.append(cam->peak_time[i]);
        pktime_b.content().end_list();
    }

    tel_ids_b.end_list();
    image_b.end_list();
    pktime_b.end_list();
}

// ---------------------------------------------------------------------------
// Per-group begin / append-one-telescope / end  helpers
// Each set of three functions is used identically for DL1 and SimTels.
// ---------------------------------------------------------------------------

inline void begin_hillas_lists(HillasRecord& r) {
    r.content<hil_length>().begin_list(); r.content<hil_width>().begin_list();
    r.content<hil_psi>().begin_list();    r.content<hil_x>().begin_list();
    r.content<hil_y>().begin_list();      r.content<hil_skewness>().begin_list();
    r.content<hil_kurtosis>().begin_list();r.content<hil_intensity>().begin_list();
    r.content<hil_r>().begin_list();      r.content<hil_phi>().begin_list();
    r.content<hil_scale_ratio>().begin_list();
}
inline void append_hillas(HillasRecord& r, const HillasParameter& h) {
    r.content<hil_length>().content().append(h.length);
    r.content<hil_width>().content().append(h.width);
    r.content<hil_psi>().content().append(h.psi);
    r.content<hil_x>().content().append(h.x);
    r.content<hil_y>().content().append(h.y);
    r.content<hil_skewness>().content().append(h.skewness);
    r.content<hil_kurtosis>().content().append(h.kurtosis);
    r.content<hil_intensity>().content().append(h.intensity);
    r.content<hil_r>().content().append(h.r);
    r.content<hil_phi>().content().append(h.phi);
    r.content<hil_scale_ratio>().content().append(h.scale_ratio);
}
inline void end_hillas_lists(HillasRecord& r) {
    r.content<hil_length>().end_list(); r.content<hil_width>().end_list();
    r.content<hil_psi>().end_list();    r.content<hil_x>().end_list();
    r.content<hil_y>().end_list();      r.content<hil_skewness>().end_list();
    r.content<hil_kurtosis>().end_list();r.content<hil_intensity>().end_list();
    r.content<hil_r>().end_list();      r.content<hil_phi>().end_list();
    r.content<hil_scale_ratio>().end_list();
}

inline void begin_leakage_lists(LeakageRecord& r) {
    r.content<leak_pixels_w1>().begin_list(); r.content<leak_pixels_w2>().begin_list();
    r.content<leak_intensity_w1>().begin_list(); r.content<leak_intensity_w2>().begin_list();
}
inline void append_leakage(LeakageRecord& r, const LeakageParameter& l) {
    r.content<leak_pixels_w1>().content().append(l.pixels_width_1);
    r.content<leak_pixels_w2>().content().append(l.pixels_width_2);
    r.content<leak_intensity_w1>().content().append(l.intensity_width_1);
    r.content<leak_intensity_w2>().content().append(l.intensity_width_2);
}
inline void end_leakage_lists(LeakageRecord& r) {
    r.content<leak_pixels_w1>().end_list(); r.content<leak_pixels_w2>().end_list();
    r.content<leak_intensity_w1>().end_list(); r.content<leak_intensity_w2>().end_list();
}

inline void begin_concentration_lists(ConcentrationRecord& r) {
    r.content<conc_cog>().begin_list();
    r.content<conc_core>().begin_list();
    r.content<conc_pixel>().begin_list();
    r.content<conc_frac2>().begin_list();
}
inline void append_concentration(ConcentrationRecord& r, const ConcentrationParameter& c) {
    r.content<conc_cog>().content().append(c.concentration_cog);
    r.content<conc_core>().content().append(c.concentration_core);
    r.content<conc_pixel>().content().append(c.concentration_pixel);
    r.content<conc_frac2>().content().append(c.concentration_frac2);
}
inline void end_concentration_lists(ConcentrationRecord& r) {
    r.content<conc_cog>().end_list();
    r.content<conc_core>().end_list();
    r.content<conc_pixel>().end_list();
    r.content<conc_frac2>().end_list();
}

inline void begin_morphology_lists(MorphologyRecord& r) {
    r.content<morph_n_pixels>().begin_list(); r.content<morph_n_islands>().begin_list();
    r.content<morph_n_small>().begin_list();  r.content<morph_n_medium>().begin_list();
    r.content<morph_n_large>().begin_list();
}
inline void append_morphology(MorphologyRecord& r, const MorphologyParameter& m) {
    r.content<morph_n_pixels>().content().append(static_cast<int32_t>(m.n_pixels));
    r.content<morph_n_islands>().content().append(static_cast<int32_t>(m.n_islands));
    r.content<morph_n_small>().content().append(static_cast<int32_t>(m.n_small_islands));
    r.content<morph_n_medium>().content().append(static_cast<int32_t>(m.n_medium_islands));
    r.content<morph_n_large>().content().append(static_cast<int32_t>(m.n_large_islands));
}
inline void end_morphology_lists(MorphologyRecord& r) {
    r.content<morph_n_pixels>().end_list(); r.content<morph_n_islands>().end_list();
    r.content<morph_n_small>().end_list();  r.content<morph_n_medium>().end_list();
    r.content<morph_n_large>().end_list();
}

inline void begin_intensity_lists(IntensityRecord& r) {
    r.content<intens_max>().begin_list();      r.content<intens_mean>().begin_list();
    r.content<intens_std>().begin_list();      r.content<intens_skewness>().begin_list();
    r.content<intens_kurtosis>().begin_list();
}
inline void append_intensity(IntensityRecord& r, const IntensityParameter& p) {
    r.content<intens_max>().content().append(p.intensity_max);
    r.content<intens_mean>().content().append(p.intensity_mean);
    r.content<intens_std>().content().append(p.intensity_std);
    r.content<intens_skewness>().content().append(p.intensity_skewness);
    r.content<intens_kurtosis>().content().append(p.intensity_kurtosis);
}
inline void end_intensity_lists(IntensityRecord& r) {
    r.content<intens_max>().end_list();      r.content<intens_mean>().end_list();
    r.content<intens_std>().end_list();      r.content<intens_skewness>().end_list();
    r.content<intens_kurtosis>().end_list();
}

inline void begin_extra_lists(ExtraRecord& r) {
    r.content<extra_miss>().begin_list();     r.content<extra_disp>().begin_list();
    r.content<extra_theta>().begin_list();    r.content<extra_true_psi>().begin_list();
    r.content<extra_cog_err>().begin_list();  r.content<extra_beta_err>().begin_list();
}
inline void append_extra(ExtraRecord& r, const ExtraParameters& e) {
    r.content<extra_miss>().content().append(e.miss);
    r.content<extra_disp>().content().append(e.disp);
    r.content<extra_theta>().content().append(e.theta);
    r.content<extra_true_psi>().content().append(e.true_psi);
    r.content<extra_cog_err>().content().append(e.cog_err);
    r.content<extra_beta_err>().content().append(e.beta_err);
}
inline void end_extra_lists(ExtraRecord& r) {
    r.content<extra_miss>().end_list();     r.content<extra_disp>().end_list();
    r.content<extra_theta>().end_list();    r.content<extra_true_psi>().end_list();
    r.content<extra_cog_err>().end_list();  r.content<extra_beta_err>().end_list();
}

/// Begin all ImageParameters list builders for a record that embeds the 6 sub-records.
/// `hillas_field`, `leakage_field`, ... are the DL1Field / SimTelField enum values.
template <std::size_t HF, std::size_t LF, std::size_t CF,
          std::size_t MF, std::size_t IF, std::size_t EF, typename REC>
inline void begin_imgparam_lists(REC& rec) {
    begin_hillas_lists(rec.template content<HF>());
    begin_leakage_lists(rec.template content<LF>());
    begin_concentration_lists(rec.template content<CF>());
    begin_morphology_lists(rec.template content<MF>());
    begin_intensity_lists(rec.template content<IF>());
    begin_extra_lists(rec.template content<EF>());
}

template <std::size_t HF, std::size_t LF, std::size_t CF,
          std::size_t MF, std::size_t IF, std::size_t EF, typename REC>
inline void append_imgparam(REC& rec, const ImageParameters& p) {
    append_hillas(rec.template content<HF>(), p.hillas);
    append_leakage(rec.template content<LF>(), p.leakage);
    append_concentration(rec.template content<CF>(), p.concentration);
    append_morphology(rec.template content<MF>(), p.morphology);
    append_intensity(rec.template content<IF>(), p.intensity);
    append_extra(rec.template content<EF>(), p.extra);
}

template <std::size_t HF, std::size_t LF, std::size_t CF,
          std::size_t MF, std::size_t IF, std::size_t EF, typename REC>
inline void end_imgparam_lists(REC& rec) {
    end_hillas_lists(rec.template content<HF>());
    end_leakage_lists(rec.template content<LF>());
    end_concentration_lists(rec.template content<CF>());
    end_morphology_lists(rec.template content<MF>());
    end_intensity_lists(rec.template content<IF>());
    end_extra_lists(rec.template content<EF>());
}

// Convenience wrappers bound to DL1Field and SimTelField indices.
inline void begin_dl1_imgparam(DL1Record& r) {
    begin_imgparam_lists<dl1_hillas, dl1_leakage, dl1_concentration,
                         dl1_morphology, dl1_intensity, dl1_extra>(r);
}
inline void append_dl1_imgparam(DL1Record& r, const ImageParameters& p) {
    append_imgparam<dl1_hillas, dl1_leakage, dl1_concentration,
                    dl1_morphology, dl1_intensity, dl1_extra>(r, p);
}
inline void end_dl1_imgparam(DL1Record& r) {
    end_imgparam_lists<dl1_hillas, dl1_leakage, dl1_concentration,
                       dl1_morphology, dl1_intensity, dl1_extra>(r);
}

inline void begin_simtel_imgparam(SimTelsRecord& r) {
    begin_imgparam_lists<simtel_hillas, simtel_leakage, simtel_concentration,
                         simtel_morphology, simtel_intensity, simtel_extra>(r);
}
inline void append_simtel_imgparam(SimTelsRecord& r, const ImageParameters& p) {
    append_imgparam<simtel_hillas, simtel_leakage, simtel_concentration,
                    simtel_morphology, simtel_intensity, simtel_extra>(r, p);
}
inline void end_simtel_imgparam(SimTelsRecord& r) {
    end_imgparam_lists<simtel_hillas, simtel_leakage, simtel_concentration,
                       simtel_morphology, simtel_intensity, simtel_extra>(r);
}

/// Fill DL1 — full ImageParameters, no pixel arrays.
inline void fill_dl1(DL1Record& rec, const DL1Event& dl1)
{
    auto& tel_ids_b = rec.content<dl1_tel_ids>();
    tel_ids_b.begin_list();
    begin_dl1_imgparam(rec);

    for (int tel_id : dl1.get_ordered_tels()) {
        const DL1Camera* cam = dl1.get_tel(tel_id);
        if (!cam) continue;
        tel_ids_b.content().append(static_cast<int32_t>(tel_id));
        append_dl1_imgparam(rec, cam->image_parameters);
    }

    tel_ids_b.end_list();
    end_dl1_imgparam(rec);
}

/// Fill the DL2 geometry sub-record for one event.
/// Returns sorted method names so the tels filler can reuse them.
inline std::vector<std::string> fill_dl2_geometry(GeometryRecord& rec,
                                                   const DL2Event& dl2_event)
{
    const auto methods = sorted_keys(dl2_event.geometry);
    append_string_list(rec.content<geom_methods>(), methods);

    auto& g_valid   = rec.content<geom_is_valid>();
    auto& g_alt     = rec.content<geom_alt>();
    auto& g_alt_unc = rec.content<geom_alt_uncertainty>();
    auto& g_az      = rec.content<geom_az>();
    auto& g_az_unc  = rec.content<geom_az_uncertainty>();
    auto& g_dir_err = rec.content<geom_direction_error>();
    auto& g_cx      = rec.content<geom_core_x>();
    auto& g_cy      = rec.content<geom_core_y>();
    auto& g_cerr    = rec.content<geom_core_pos_error>();
    auto& g_hmax    = rec.content<geom_hmax>();
    auto& g_xmax    = rec.content<geom_xmax>();
    auto& g_tels    = rec.content<geom_tel_ids>();

    g_valid.begin_list(); g_alt.begin_list(); g_alt_unc.begin_list();
    g_az.begin_list();    g_az_unc.begin_list(); g_dir_err.begin_list();
    g_cx.begin_list();    g_cy.begin_list(); g_cerr.begin_list();
    g_hmax.begin_list();  g_xmax.begin_list(); g_tels.begin_list();

    for (const auto& name : methods) {
        const auto& g = dl2_event.geometry.at(name);
        g_valid.content().append(static_cast<int8_t>(g.is_valid ? 1 : 0));
        g_alt.content().append(g.alt);
        g_alt_unc.content().append(g.alt_uncertainty);
        g_az.content().append(g.az);
        g_az_unc.content().append(g.az_uncertainty);
        g_dir_err.content().append(g.direction_error);
        g_cx.content().append(g.core_x);
        g_cy.content().append(g.core_y);
        g_cerr.content().append(g.core_pos_error);
        g_hmax.content().append(g.hmax);
        g_xmax.content().append(g.xmax);

        auto& tels_inner = g_tels.content().begin_list();
        for (int tid : g.telescopes)
            tels_inner.append(static_cast<int32_t>(tid));
        g_tels.content().end_list();
    }

    g_valid.end_list(); g_alt.end_list(); g_alt_unc.end_list();
    g_az.end_list();    g_az_unc.end_list(); g_dir_err.end_list();
    g_cx.end_list();    g_cy.end_list(); g_cerr.end_list();
    g_hmax.end_list();  g_xmax.end_list(); g_tels.end_list();

    return methods;
}

inline void fill_dl2_energy(EnergyRecord& rec, const DL2Event& dl2_event)
{
    const auto methods = sorted_keys(dl2_event.energy);
    append_string_list(rec.content<energy_methods>(), methods);

    auto& e_valid = rec.content<energy_valid>();
    auto& e_est   = rec.content<energy_estimate>();
    auto& e_std   = rec.content<energy_estimate_std>();
    auto& e_tels  = rec.content<energy_tel_ids>();

    e_valid.begin_list(); e_est.begin_list();
    e_std.begin_list();   e_tels.begin_list();

    for (const auto& name : methods) {
        const auto& en = dl2_event.energy.at(name);
        e_valid.content().append(static_cast<int8_t>(en.energy_valid ? 1 : 0));
        e_est.content().append(en.estimate_energy);
        e_std.content().append(en.estimate_energy_std);
        auto& tels_inner = e_tels.content().begin_list();
        for (int tid : en.telescopes)
            tels_inner.append(static_cast<int32_t>(tid));
        e_tels.content().end_list();
    }

    e_valid.end_list(); e_est.end_list();
    e_std.end_list();   e_tels.end_list();
}

inline void fill_dl2_particle(ParticleRecord& rec, const DL2Event& dl2_event)
{
    const auto methods = sorted_keys(dl2_event.particle);
    append_string_list(rec.content<part_methods>(), methods);

    auto& p_valid = rec.content<part_valid>();
    auto& p_hadro = rec.content<part_hadroness>();
    auto& p_mrsl  = rec.content<part_mrsl>();
    auto& p_mrsw  = rec.content<part_mrsw>();
    auto& p_tels  = rec.content<part_tel_ids>();

    p_valid.begin_list(); p_hadro.begin_list(); p_mrsl.begin_list();
    p_mrsw.begin_list();  p_tels.begin_list();

    for (const auto& name : methods) {
        const auto& pt = dl2_event.particle.at(name);
        p_valid.content().append(static_cast<int8_t>(pt.is_valid ? 1 : 0));
        p_hadro.content().append(pt.hadroness);
        p_mrsl.content().append(pt.mrsl);
        p_mrsw.content().append(pt.mrsw);
        auto& tels_inner = p_tels.content().begin_list();
        for (int tid : pt.telescopes)
            tels_inner.append(static_cast<int32_t>(tid));
        p_tels.content().end_list();
    }

    p_valid.end_list(); p_hadro.end_list(); p_mrsl.end_list();
    p_mrsw.end_list();  p_tels.end_list();
}

inline void fill_dl2_tels(DL2TelsRecord& rec, const DL2Event& dl2_event,
                           const std::vector<std::string>& geom_methods)
{
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    std::vector<int> sorted_tel_ids;
    sorted_tel_ids.reserve(dl2_event.tels.size());
    for (const auto& [k, _] : dl2_event.tels) sorted_tel_ids.push_back(k);
    std::sort(sorted_tel_ids.begin(), sorted_tel_ids.end());

    auto& t_ids     = rec.content<dl2tel_ids>();
    auto& t_energy  = rec.content<dl2tel_estimate_energy>();
    auto& t_hadro   = rec.content<dl2tel_estimate_hadroness>();
    auto& t_disp    = rec.content<dl2tel_estimate_disp>();
    auto& t_imp     = rec.content<dl2tel_impact_distance>();
    auto& t_imp_err = rec.content<dl2tel_impact_distance_error>();

    t_ids.begin_list(); t_energy.begin_list(); t_hadro.begin_list();
    t_disp.begin_list(); t_imp.begin_list(); t_imp_err.begin_list();

    for (int tel_id : sorted_tel_ids) {
        const auto& tel = dl2_event.tels.at(tel_id);
        t_ids.content().append(static_cast<int32_t>(tel_id));
        t_energy.content().append(tel.estimate_energy);
        t_hadro.content().append(tel.estimate_hadroness);
        t_disp.content().append(tel.estimate_disp);

        // Impact distances aligned with geom_methods order
        auto& imp_inner     = t_imp.content().begin_list();
        auto& imp_err_inner = t_imp_err.content().begin_list();
        for (const auto& gm : geom_methods) {
            auto it = tel.impact_parameters.find(gm);
            if (it != tel.impact_parameters.end()) {
                imp_inner.append(it->second.distance);
                imp_err_inner.append(it->second.distance_error);
            } else {
                imp_inner.append(nan);
                imp_err_inner.append(nan);
            }
        }
        t_imp.content().end_list();
        t_imp_err.content().end_list();
    }

    t_ids.end_list(); t_energy.end_list(); t_hadro.end_list();
    t_disp.end_list(); t_imp.end_list(); t_imp_err.end_list();
}

inline void fill_dl2(DL2Record& rec, const DL2Event& dl2_event)
{
    // geometry returns the sorted method names for tels alignment
    const auto geom_methods = fill_dl2_geometry(rec.content<dl2_geometry>(), dl2_event);
    fill_dl2_energy(rec.content<dl2_energy>(), dl2_event);
    fill_dl2_particle(rec.content<dl2_particle>(), dl2_event);
    fill_dl2_tels(rec.content<dl2_tels>(), dl2_event, geom_methods);
}

/// Fill SimulatedCamera per-telescope scalars + full ImageParameters.
/// Per-pixel images (true_image, fake_image) are excluded intentionally.
inline void fill_sim_tels(SimTelsRecord& rec, const SimulatedEvent& sim_event)
{
    auto& ids_b  = rec.content<simtel_ids>();
    auto& sum_b  = rec.content<simtel_true_image_sum>();
    auto& imp_b  = rec.content<simtel_impact_parameter>();
    auto& time_b = rec.content<simtel_time_range_10_90>();

    ids_b.begin_list(); sum_b.begin_list();
    imp_b.begin_list(); time_b.begin_list();
    begin_simtel_imgparam(rec);

    for (int tel_id : sim_event.get_ordered_tels()) {
        const SimulatedCamera* cam = sim_event.get_tel(tel_id);
        if (!cam) continue;

        ids_b.content().append(static_cast<int32_t>(tel_id));
        sum_b.content().append(static_cast<int32_t>(cam->true_image_sum));
        imp_b.content().append(cam->impact_parameter);
        time_b.content().append(cam->time_range_10_90);
        append_simtel_imgparam(rec, cam->image_parameters);
    }

    ids_b.end_list(); sum_b.end_list();
    imp_b.end_list(); time_b.end_list();
    end_simtel_imgparam(rec);
}

/// Fill the simulation sub-record (shower scalars + triggered_tels + per-tel cameras).
inline void fill_simulation(SimRecord& rec, const SimulatedEvent& sim_event)
{
    const auto& shower = sim_event.shower;
    rec.content<sim_energy>().append(shower.energy);
    rec.content<sim_alt>().append(shower.alt);
    rec.content<sim_az>().append(shower.az);
    rec.content<sim_core_x>().append(shower.core_x);
    rec.content<sim_core_y>().append(shower.core_y);
    rec.content<sim_h_first_int>().append(shower.h_first_int);
    rec.content<sim_x_max>().append(shower.x_max);
    rec.content<sim_h_max>().append(shower.h_max);
    rec.content<sim_starting_grammage>().append(shower.starting_grammage);
    rec.content<sim_primary_id>().append(static_cast<int32_t>(shower.shower_primary_id));

    auto& trig_b = rec.content<sim_triggered_tels>();
    trig_b.begin_list();
    for (int tid : sim_event.triggered_tels)
        trig_b.content().append(static_cast<int32_t>(tid));
    trig_b.end_list();

    fill_sim_tels(rec.content<sim_tels>(), sim_event);
}

} // namespace ak_builder_detail


// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * @brief Accumulates ArrayEvents into a single unified nested ak.Array.
 *
 * Usage:
 *   ArrayEventBuilder aeb;
 *   for (auto& ev : source) aeb.append(ev);
 *   nb::object arr = snapshot_builder(aeb.builder);  // see binding
 */
class ArrayEventBuilder {
public:
    ak_builder_detail::TopRecordBuilder builder;

    ArrayEventBuilder() {
        using namespace ak_builder_detail;
        using Map = UserDefinedMap;

        // --- top level ---
        builder.set_fields(Map{
            {ev_event_id,   "event_id"},
            {ev_run_id,     "run_id"},
            {ev_mjd,        "mjd"},
            {ev_simulation, "simulation"},
            {ev_pointing,   "pointing"},
            {ev_dl0,        "dl0"},
            {ev_dl1,        "dl1"},
            {ev_dl2,        "dl2"}
        });

        // --- simulation ---
        auto& sim_rec = builder.content<ev_simulation>().content();
        sim_rec.set_fields(Map{
            {sim_energy,            "energy"},
            {sim_alt,               "alt"},
            {sim_az,                "az"},
            {sim_core_x,            "core_x"},
            {sim_core_y,            "core_y"},
            {sim_h_first_int,       "h_first_int"},
            {sim_x_max,             "x_max"},
            {sim_h_max,             "h_max"},
            {sim_starting_grammage, "starting_grammage"},
            {sim_primary_id,        "shower_primary_id"},
            {sim_triggered_tels,    "triggered_tels"},
            {sim_tels,              "tels"}
        });
        auto& simtels_rec = sim_rec.content<sim_tels>();
        simtels_rec.set_fields(Map{
            {simtel_ids,              "ids"},
            {simtel_true_image_sum,   "true_image_sum"},
            {simtel_impact_parameter, "impact_parameter"},
            {simtel_time_range_10_90, "time_range_10_90"},
            {simtel_hillas,           "hillas"},
            {simtel_leakage,          "leakage"},
            {simtel_concentration,    "concentration"},
            {simtel_morphology,       "morphology"},
            {simtel_intensity,        "intensity"},
            {simtel_extra,            "extra"}
        });
        simtels_rec.content<simtel_hillas>().set_fields(Map{
            {hil_length, "length"}, {hil_width, "width"}, {hil_psi, "psi"},
            {hil_x, "x"}, {hil_y, "y"},
            {hil_skewness, "skewness"}, {hil_kurtosis, "kurtosis"},
            {hil_intensity, "intensity"},
            {hil_r, "r"}, {hil_phi, "phi"}, {hil_scale_ratio, "scale_ratio"}
        });
        simtels_rec.content<simtel_leakage>().set_fields(Map{
            {leak_pixels_w1, "pixels_width_1"}, {leak_pixels_w2, "pixels_width_2"},
            {leak_intensity_w1, "intensity_width_1"}, {leak_intensity_w2, "intensity_width_2"}
        });
        simtels_rec.content<simtel_concentration>().set_fields(Map{
            {conc_cog, "cog"}, {conc_core, "core"}, {conc_pixel, "pixel"}, {conc_frac2, "frac2"}
        });
        simtels_rec.content<simtel_morphology>().set_fields(Map{
            {morph_n_pixels, "n_pixels"}, {morph_n_islands, "n_islands"},
            {morph_n_small, "n_small_islands"}, {morph_n_medium, "n_medium_islands"},
            {morph_n_large, "n_large_islands"}
        });
        simtels_rec.content<simtel_intensity>().set_fields(Map{
            {intens_max, "max"}, {intens_mean, "mean"}, {intens_std, "std"},
            {intens_skewness, "skewness"}, {intens_kurtosis, "kurtosis"}
        });
        simtels_rec.content<simtel_extra>().set_fields(Map{
            {extra_miss, "miss"}, {extra_disp, "disp"}, {extra_theta, "theta"},
            {extra_true_psi, "true_psi"}, {extra_cog_err, "cog_err"},
            {extra_beta_err, "beta_err"}
        });

        // --- pointing ---
        builder.content<ev_pointing>().content().set_fields(Map{
            {pt_array_azimuth,  "array_azimuth"},
            {pt_array_altitude, "array_altitude"},
            {pt_tel_ids,        "tel_ids"},
            {pt_tel_azimuth,    "tel_azimuth"},
            {pt_tel_altitude,   "tel_altitude"}
        });

        // --- dl0 ---
        builder.content<ev_dl0>().content().set_fields(Map{
            {dl0_tel_ids,   "tel_ids"},
            {dl0_image,     "image"},
            {dl0_peak_time, "peak_time"}
        });

        // --- dl1 ---  (image/peak_time/mask excluded — O(n_pixels), handle in C++)
        auto& dl1_rec = builder.content<ev_dl1>().content();
        dl1_rec.set_fields(Map{
            {dl1_tel_ids,       "tel_ids"},
            {dl1_hillas,        "hillas"},
            {dl1_leakage,       "leakage"},
            {dl1_concentration, "concentration"},
            {dl1_morphology,    "morphology"},
            {dl1_intensity,     "intensity"},
            {dl1_extra,         "extra"}
        });
        dl1_rec.content<dl1_hillas>().set_fields(Map{
            {hil_length, "length"}, {hil_width, "width"}, {hil_psi, "psi"},
            {hil_x, "x"}, {hil_y, "y"},
            {hil_skewness, "skewness"}, {hil_kurtosis, "kurtosis"},
            {hil_intensity, "intensity"},
            {hil_r, "r"}, {hil_phi, "phi"}, {hil_scale_ratio, "scale_ratio"}
        });
        dl1_rec.content<dl1_leakage>().set_fields(Map{
            {leak_pixels_w1, "pixels_width_1"}, {leak_pixels_w2, "pixels_width_2"},
            {leak_intensity_w1, "intensity_width_1"}, {leak_intensity_w2, "intensity_width_2"}
        });
        dl1_rec.content<dl1_concentration>().set_fields(Map{
            {conc_cog, "cog"}, {conc_core, "core"}, {conc_pixel, "pixel"},
            {conc_frac2, "frac2"}
        });
        dl1_rec.content<dl1_morphology>().set_fields(Map{
            {morph_n_pixels, "n_pixels"}, {morph_n_islands, "n_islands"},
            {morph_n_small, "n_small_islands"}, {morph_n_medium, "n_medium_islands"},
            {morph_n_large, "n_large_islands"}
        });
        dl1_rec.content<dl1_intensity>().set_fields(Map{
            {intens_max, "max"}, {intens_mean, "mean"}, {intens_std, "std"},
            {intens_skewness, "skewness"}, {intens_kurtosis, "kurtosis"}
        });
        dl1_rec.content<dl1_extra>().set_fields(Map{
            {extra_miss, "miss"}, {extra_disp, "disp"}, {extra_theta, "theta"},
            {extra_true_psi, "true_psi"}, {extra_cog_err, "cog_err"},
            {extra_beta_err, "beta_err"}
        });

        // --- dl2 ---
        auto& dl2_rec = builder.content<ev_dl2>().content();
        dl2_rec.set_fields(Map{
            {dl2_geometry, "geometry"},
            {dl2_energy,   "energy"},
            {dl2_particle, "particle"},
            {dl2_tels,     "tels"}
        });

        // dl2.geometry
        auto& geom_rec = dl2_rec.content<dl2_geometry>();
        geom_rec.set_fields(Map{
            {geom_methods,          "methods"},
            {geom_is_valid,         "is_valid"},
            {geom_alt,              "alt"},
            {geom_alt_uncertainty,  "alt_uncertainty"},
            {geom_az,               "az"},
            {geom_az_uncertainty,   "az_uncertainty"},
            {geom_direction_error,  "direction_error"},
            {geom_core_x,           "core_x"},
            {geom_core_y,           "core_y"},
            {geom_core_pos_error,   "core_pos_error"},
            {geom_hmax,             "hmax"},
            {geom_xmax,             "xmax"},
            {geom_tel_ids,          "tel_ids"}
        });
        mark_string_builder(geom_rec.content<geom_methods>());

        // dl2.energy
        auto& energy_rec = dl2_rec.content<dl2_energy>();
        energy_rec.set_fields(Map{
            {energy_methods,      "methods"},
            {energy_valid,        "energy_valid"},
            {energy_estimate,     "estimate"},
            {energy_estimate_std, "estimate_std"},
            {energy_tel_ids,      "tel_ids"}
        });
        mark_string_builder(energy_rec.content<energy_methods>());

        // dl2.particle
        auto& part_rec = dl2_rec.content<dl2_particle>();
        part_rec.set_fields(Map{
            {part_methods,   "methods"},
            {part_valid,     "is_valid"},
            {part_hadroness, "hadroness"},
            {part_mrsl,      "mrsl"},
            {part_mrsw,      "mrsw"},
            {part_tel_ids,   "tel_ids"}
        });
        mark_string_builder(part_rec.content<part_methods>());

        // dl2.tels
        dl2_rec.content<dl2_tels>().set_fields(Map{
            {dl2tel_ids,                   "ids"},
            {dl2tel_estimate_energy,       "estimate_energy"},
            {dl2tel_estimate_hadroness,    "estimate_hadroness"},
            {dl2tel_estimate_disp,         "estimate_disp"},
            {dl2tel_impact_distance,       "impact_distance"},
            {dl2tel_impact_distance_error, "impact_distance_error"}
        });
    }

    /// @brief Appends one ArrayEvent to all builders.
    void append(const ArrayEvent& event) {
        using namespace ak_builder_detail;

        builder.content<ev_event_id>().append(static_cast<int32_t>(event.event_id));
        builder.content<ev_run_id>().append(static_cast<int32_t>(event.run_id));

        // mjd
        builder.content<ev_mjd>().append(event.mjd->to_float());

        // simulation
        if (event.simulation.has_value()) {
            auto& sim = builder.content<ev_simulation>().append_valid();
            fill_simulation(sim, *event.simulation);
        } else {
            builder.content<ev_simulation>().append_invalid();
        }

        // pointing
        if (event.pointing.has_value()) {
            auto& pt = builder.content<ev_pointing>().append_valid();
            const auto& pointing = *event.pointing;
            pt.content<pt_array_azimuth>().append(pointing.array_azimuth);
            pt.content<pt_array_altitude>().append(pointing.array_altitude);

            auto& tel_ids_b = pt.content<pt_tel_ids>();
            auto& tel_az_b  = pt.content<pt_tel_azimuth>();
            auto& tel_alt_b = pt.content<pt_tel_altitude>();

            tel_ids_b.begin_list();
            tel_az_b.begin_list();
            tel_alt_b.begin_list();

            for (int tel_id : pointing.get_ordered_tels()) {
                const PointingTelescope* tel = pointing.get_tel(tel_id);
                if (!tel) continue;
                tel_ids_b.content().append(static_cast<int32_t>(tel_id));
                tel_az_b.content().append(tel->azimuth);
                tel_alt_b.content().append(tel->altitude);
            }

            tel_ids_b.end_list();
            tel_az_b.end_list();
            tel_alt_b.end_list();
        } else {
            builder.content<ev_pointing>().append_invalid();
        }

        // dl0
        if (event.dl0.has_value()) {
            auto& dl0_rec = builder.content<ev_dl0>().append_valid();
            fill_dl0(dl0_rec, *event.dl0);
        } else {
            builder.content<ev_dl0>().append_invalid();
        }

        // dl1
        if (event.dl1.has_value()) {
            auto& dl1_rec = builder.content<ev_dl1>().append_valid();
            fill_dl1(dl1_rec, *event.dl1);
        } else {
            builder.content<ev_dl1>().append_invalid();
        }

        // dl2
        if (event.dl2.has_value()) {
            auto& dl2_rec = builder.content<ev_dl2>().append_valid();
            fill_dl2(dl2_rec, *event.dl2);
        } else {
            builder.content<ev_dl2>().append_invalid();
        }
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return builder.length();
    }
};

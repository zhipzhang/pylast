#pragma once
#include "ArrayEvent.hh"
#include "EventSource.hh"
#include <arrow/api.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace df {
    template <typename Tuple, typename F, std::size_t... I>
    auto for_each_impl(Tuple &&t, F &&f, std::index_sequence<I...>) {
        return (void)std::initializer_list<int>{
            (f(std::get<I>(std::forward<Tuple>(t))), 0)...};
    }

    template <typename Tuple, typename F>
    auto for_each_tuple(Tuple &&t, F &&f) {
        constexpr auto N = std::tuple_size<std::decay_t<Tuple>>::value;
        for_each_impl(std::forward<Tuple>(t), std::forward<F>(f),
                      std::make_index_sequence<N>{});
    }

    struct FieldMeta {
        const char *name;
        std::shared_ptr<arrow::DataType> type;
    };

    template <typename Derived, size_t N>
    struct BaseData {
        BaseData() = default;
        void reserve_builders(size_t reserve_rows = 0) {
            auto builders = static_cast<Derived *>(this)->builders();
            std::apply([&](auto &...b) { (b.Reserve(reserve_rows), ...); },
                       builders);
        }

        static std::shared_ptr<arrow::Schema> make_schema() {
            const auto &meta = Derived::field_meta();
            std::vector<std::shared_ptr<arrow::Field>> fields;
            fields.reserve(N);
            for (auto &m : meta) {
                fields.push_back(arrow::field(m.name, m.type));
            }
            return arrow::schema(fields);
        }

        arrow::Result<std::shared_ptr<arrow::Table>> finish_builders() {
            auto builders = static_cast<Derived *>(this)->builders();
            std::vector<std::shared_ptr<arrow::Array>> arrays;
            arrays.reserve(N);

            for_each_tuple(builders, [&](auto &b) -> arrow::Status {
                std::shared_ptr<arrow::Array> arr;
                ARROW_RETURN_NOT_OK(b.Finish(&arr));
                arrays.push_back(std::move(arr));
                return arrow::Status::OK();
            });

            return arrow::Table::Make(Derived::make_schema(), arrays);
        }
    };

    struct SimulationData : public BaseData<SimulationData, 10> {
        using Base = BaseData<SimulationData, 10>;

        SimulationData(size_t reserve_rows = 0) {
            if (reserve_rows == 0)
                reserve_rows = 1000;
            this->reserve_builders(reserve_rows);
        }

        static auto field_meta() {
            return std::array{FieldMeta{"event_id", arrow::int32()},
                              FieldMeta{"true_energy", arrow::float64()},
                              FieldMeta{"true_alt", arrow::float64()},
                              FieldMeta{"true_az", arrow::float64()},
                              FieldMeta{"true_core_x", arrow::float64()},
                              FieldMeta{"true_core_y", arrow::float64()},
                              FieldMeta{"h_first_int", arrow::float64()},
                              FieldMeta{"x_max", arrow::float64()},
                              FieldMeta{"h_max", arrow::float64()},
                              FieldMeta{"starting_grammage", arrow::float64()}};
        }

        auto builders() {
            return std::tie(event_id, true_energy, true_alt, true_az,
                            true_core_x, true_core_y, h_first_int, x_max, h_max,
                            starting_grammage);
        }

        // --- 写数据 ---
        void write_event(const ArrayEvent &event) {
            const auto &shower = event.simulation->shower;
            event_id.Append(event.event_id);
            true_energy.Append(shower.energy);
            true_alt.Append(shower.alt);
            true_az.Append(shower.az);
            true_core_x.Append(shower.core_x);
            true_core_y.Append(shower.core_y);
            h_first_int.Append(shower.h_first_int);
            x_max.Append(shower.x_max);
            h_max.Append(shower.h_max);
            starting_grammage.Append(shower.starting_grammage);
        }

        arrow::Int32Builder event_id;
        arrow::DoubleBuilder true_energy, true_alt, true_az;
        arrow::DoubleBuilder true_core_x, true_core_y;
        arrow::DoubleBuilder h_first_int, x_max, h_max, starting_grammage;
    };

    struct ReconstructorData : public BaseData<ReconstructorData, 16> {
        using Base = BaseData<ReconstructorData, 16>;

        ReconstructorData(size_t reserve_rows = 0) {
            if (reserve_rows == 0)
                reserve_rows = 1000;
            this->reserve_builders(reserve_rows);
        }

        static auto field_meta() {
            return std::array{
                FieldMeta{"reconstructor_name",
                          arrow::dictionary(arrow::int8(), arrow::utf8())},
                FieldMeta{"event_id", arrow::int32()},
                FieldMeta{"is_valid", arrow::boolean()},
                FieldMeta{"rec_alt", arrow::float64()},
                FieldMeta{"rec_alt_uncertainty", arrow::float64()},
                FieldMeta{"rec_az", arrow::float64()},
                FieldMeta{"rec_az_uncertainty", arrow::float64()},
                FieldMeta{"rec_core_x", arrow::float64()},
                FieldMeta{"rec_core_y", arrow::float64()},
                FieldMeta{"rec_core_position_error", arrow::float64()},
                FieldMeta{"tilted_core_x", arrow::float64()},
                FieldMeta{"tilted_core_y", arrow::float64()},
                FieldMeta{"tilted_core_uncertainty_x", arrow::float64()},
                FieldMeta{"tilted_core_uncertainty_y", arrow::float64()},
                FieldMeta{"hmax", arrow::float64()},
                FieldMeta{"direction_error", arrow::float64()}};
        }

        auto builders() {
            return std::tie(reconstructor_name, event_id, is_valid, rec_alt,
                            rec_alt_uncertainty, rec_az, rec_az_uncertainty,
                            rec_core_x, rec_core_y, rec_core_position_error,
                            tilted_core_x, tilted_core_y,
                            tilted_core_uncertainty_x,
                            tilted_core_uncertainty_y, hmax, direction_error);
        }

        void write_event(const ArrayEvent &event) {
            const auto &geometry = event.dl2->geometry;
            for (const auto &[name, recon] : geometry) {
                reconstructor_name.Append(name);
                event_id.Append(event.event_id);
                is_valid.Append(recon.is_valid);
                rec_alt.Append(recon.alt);
                rec_alt_uncertainty.Append(recon.alt_uncertainty);
                rec_az.Append(recon.az);
                rec_az_uncertainty.Append(recon.az_uncertainty);
                rec_core_x.Append(recon.core_x);
                rec_core_y.Append(recon.core_y);
                rec_core_position_error.Append(recon.core_pos_error);
                tilted_core_x.Append(recon.tilted_core_x);
                tilted_core_y.Append(recon.tilted_core_y);
                tilted_core_uncertainty_x.Append(
                    recon.tilted_core_uncertainty_x);
                tilted_core_uncertainty_y.Append(
                    recon.tilted_core_uncertainty_y);
                hmax.Append(recon.hmax);
                direction_error.Append(recon.direction_error);
            }
        }

        arrow::DictionaryBuilder<arrow::StringType> reconstructor_name;
        arrow::Int32Builder event_id;
        arrow::BooleanBuilder is_valid;
        arrow::DoubleBuilder rec_alt, rec_alt_uncertainty;
        arrow::DoubleBuilder rec_az, rec_az_uncertainty;
        arrow::DoubleBuilder rec_core_x, rec_core_y;
        arrow::DoubleBuilder rec_core_position_error;
        arrow::DoubleBuilder tilted_core_x, tilted_core_y;
        arrow::DoubleBuilder tilted_core_uncertainty_x,
            tilted_core_uncertainty_y;
        arrow::DoubleBuilder hmax, direction_error;
    };

    struct TelescopeData : public BaseData<TelescopeData, 33> {
        using Base = BaseData<TelescopeData, 33>;

        TelescopeData(size_t reserve_rows = 0) {
            if (reserve_rows == 0)
                reserve_rows = 1000;
            this->reserve_builders(reserve_rows);
        }

        static auto field_meta() {
            return std::array{
                FieldMeta{"event_id", arrow::int32()},
                FieldMeta{"tel_id", arrow::int32()},
                FieldMeta{"hillas_length", arrow::float64()},
                FieldMeta{"hillas_width", arrow::float64()},
                FieldMeta{"hillas_psi", arrow::float64()},
                FieldMeta{"hillas_x", arrow::float64()},
                FieldMeta{"hillas_y", arrow::float64()},
                FieldMeta{"hillas_skewness", arrow::float64()},
                FieldMeta{"hillas_kurtosis", arrow::float64()},
                FieldMeta{"hillas_intensity", arrow::float64()},
                FieldMeta{"hillas_r", arrow::float64()},
                FieldMeta{"hillas_phi", arrow::float64()},
                FieldMeta{"leakage_pixels_width_1", arrow::float64()},
                FieldMeta{"leakage_pixels_width_2", arrow::float64()},
                FieldMeta{"leakage_intensity_width_1", arrow::float64()},
                FieldMeta{"leakage_intensity_width_2", arrow::float64()},
                FieldMeta{"concentration_cog", arrow::float64()},
                FieldMeta{"concentration_core", arrow::float64()},
                FieldMeta{"concentration_pixel", arrow::float64()},
                FieldMeta{"intensity_max", arrow::float64()},
                FieldMeta{"intensity_mean", arrow::float64()},
                FieldMeta{"intensity_std", arrow::float64()},
                FieldMeta{"intensity_skewness", arrow::float64()},
                FieldMeta{"intensity_kurtosis", arrow::float64()},
                FieldMeta{"extra_miss", arrow::float64()},
                FieldMeta{"extra_disp", arrow::float64()},
                FieldMeta{"extra_theta", arrow::float64()},
                FieldMeta{"extra_true_psi", arrow::float64()},
                FieldMeta{"extra_cog_err", arrow::float64()},
                FieldMeta{"extra_beta_err", arrow::float64()},
                FieldMeta{"morphology_n_pixels", arrow::int32()},
                FieldMeta{"morphology_n_islands", arrow::int32()},
                FieldMeta{"morphology_n_small_islands", arrow::int32()},
                FieldMeta{"morphology_n_medium_islands", arrow::int32()},
                FieldMeta{"morphology_n_large_islands", arrow::int32()}};
        }

        auto builders() {
            return std::tie(
                event_id, tel_id, hillas_length, hillas_width, hillas_psi,
                hillas_x, hillas_y, hillas_skewness, hillas_kurtosis,
                hillas_intensity, hillas_r, hillas_phi, leakage_pixels_width_1,
                leakage_pixels_width_2, leakage_intensity_width_1,
                leakage_intensity_width_2, concentration_cog,
                concentration_core, concentration_pixel, intensity_max,
                intensity_mean, intensity_std, intensity_skewness,
                intensity_kurtosis, extra_miss, extra_disp, extra_theta,
                extra_true_psi, extra_cog_err, extra_beta_err,
                morphology_n_pixels, morphology_n_islands,
                morphology_n_small_islands, morphology_n_medium_islands,
                morphology_n_large_islands);
        }

        void write_event(const ArrayEvent &event) {
            if (!event.dl1.has_value()) {
                return;
            }
            const auto &dl1 = event.dl1;
            for (const auto &[index, tel] : dl1->tels) {
                event_id.Append(event.event_id);
                tel_id.Append(index);

                const auto &h = tel->image_parameters.hillas;
                hillas_length.Append(h.length);
                hillas_width.Append(h.width);
                hillas_psi.Append(h.psi);
                hillas_x.Append(h.x);
                hillas_y.Append(h.y);
                hillas_intensity.Append(h.intensity);
                hillas_skewness.Append(h.skewness);
                hillas_kurtosis.Append(h.kurtosis);
                hillas_r.Append(h.r);
                hillas_phi.Append(h.phi);

                const auto &l = tel->image_parameters.leakage;
                leakage_pixels_width_1.Append(l.pixels_width_1);
                leakage_pixels_width_2.Append(l.pixels_width_2);
                leakage_intensity_width_1.Append(l.intensity_width_1);
                leakage_intensity_width_2.Append(l.intensity_width_2);

                const auto &c = tel->image_parameters.concentration;
                concentration_cog.Append(c.concentration_cog);
                concentration_core.Append(c.concentration_core);
                concentration_pixel.Append(c.concentration_pixel);

                const auto &i = tel->image_parameters.intensity;
                intensity_max.Append(i.intensity_max);
                intensity_mean.Append(i.intensity_mean);
                intensity_std.Append(i.intensity_std);
                intensity_skewness.Append(i.intensity_skewness);
                intensity_kurtosis.Append(i.intensity_kurtosis);

                const auto &e = tel->image_parameters.extra;
                extra_miss.Append(e.miss);
                extra_disp.Append(e.disp);
                extra_theta.Append(e.theta);
                extra_true_psi.Append(e.true_psi);
                extra_cog_err.Append(e.cog_err);
                extra_beta_err.Append(e.beta_err);

                const auto &m = tel->image_parameters.morphology;
                morphology_n_pixels.Append(m.n_pixels);
                morphology_n_islands.Append(m.n_islands);
                morphology_n_small_islands.Append(m.n_small_islands);
                morphology_n_medium_islands.Append(m.n_medium_islands);
                morphology_n_large_islands.Append(m.n_large_islands);
            }
        }
        
        arrow::Int32Builder event_id, tel_id;
        arrow::DoubleBuilder hillas_length, hillas_width, hillas_psi;
        arrow::DoubleBuilder hillas_x, hillas_y;
        arrow::DoubleBuilder hillas_skewness, hillas_kurtosis;
        arrow::DoubleBuilder hillas_intensity;
        arrow::DoubleBuilder hillas_r, hillas_phi;
        arrow::DoubleBuilder leakage_pixels_width_1, leakage_pixels_width_2;
        arrow::DoubleBuilder leakage_intensity_width_1,
            leakage_intensity_width_2;
        arrow::DoubleBuilder concentration_cog, concentration_core,
            concentration_pixel;
        arrow::DoubleBuilder intensity_max, intensity_mean, intensity_std,
            intensity_skewness, intensity_kurtosis;
        arrow::DoubleBuilder extra_miss, extra_disp, extra_theta,
            extra_true_psi, extra_cog_err, extra_beta_err;
        arrow::Int32Builder morphology_n_pixels, morphology_n_islands,
            morphology_n_small_islands, morphology_n_medium_islands,
            morphology_n_large_islands;
    };

    template <typename TableType>
    class TableMaker {
    public:
        TableMaker(size_t reserve_rows = 0) : _data(reserve_rows) {}

        void write_event(const ArrayEvent &event) { _data.write_event(event); }

        std::shared_ptr<arrow::Table> make_table() {
            auto result = _data.finish_builders();

            if (!result.ok()) {
                std::cerr << "Failed to finish builders: " << result.status()
                          << std::endl;
                return nullptr;
            }

            std::shared_ptr<arrow::Table> table = *result;
            return table;
        }

    private:
        TableType _data;
        std::vector<std::shared_ptr<arrow::Array>> _arrays;
    };

    struct DataTable {
        std::shared_ptr<arrow::Table> simulation_table;
        std::shared_ptr<arrow::Table> reconstructor_table;
        std::shared_ptr<arrow::Table> telescope_table;
    };

    class DataFrameMaker {
    public:
        DataFrameMaker(EventSource &source, size_t reserve_rows = 100000)
            : _source(source), _simulation_table_maker(reserve_rows),
              _reconstructor_table_maker(reserve_rows),
              _telescope_table_maker(reserve_rows){};

        // std::shared_ptr<arrow::Table> operator()();
        DataTable operator()() { return build_table(); }

        DataTable build_table() {
            for (const auto &event : _source) {
                _simulation_table_maker.write_event(event);
                _reconstructor_table_maker.write_event(event);
                _telescope_table_maker.write_event(event);
            }
            auto simulation_table = _simulation_table_maker.make_table();
            auto reconstructor_table = _reconstructor_table_maker.make_table();
            auto telescope_table = _telescope_table_maker.make_table();
            return DataTable{simulation_table, reconstructor_table,
                             telescope_table};
        }

    private:
        EventSource &_source;
        TableMaker<SimulationData> _simulation_table_maker;
        TableMaker<ReconstructorData> _reconstructor_table_maker;
        TableMaker<TelescopeData> _telescope_table_maker;
    };

    // 下面是多线程的东西，但是因为 EventSource 里面的迭代器不是线程安全的
    // 所以基本还是要加锁，结果基本没啥太大变化

    class ThreadSafeEventSource {
    public:
        explicit ThreadSafeEventSource(EventSource &src)
            : src_(src), it_(src_.begin()), end_(src_.end()) {}

        std::optional<ArrayEvent> next_event() {
            std::lock_guard<std::mutex> lk(mtx_);
            if (it_ == end_)
                return std::nullopt;
            auto ev = std::move(*it_);
            ++it_;
            return ev;
        }

    private:
        EventSource &src_;
        EventSource::Iterator it_;
        EventSource::Iterator end_;
        std::mutex mtx_;
    };

    class MultiThreadedDataFrameMaker {
    public:
        MultiThreadedDataFrameMaker(EventSource &source,
                                    size_t reserve_rows = 100000)
            : _source(source), _simulation_table_maker(reserve_rows),
              _reconstructor_table_maker(reserve_rows),
              _telescope_table_maker(reserve_rows) {}

        DataTable operator()() { return build_table(); }

        DataTable build_table() {
            ThreadSafeEventSource ts_source(_source);

            // 三个 worker：每个线程不断拉取事件并写入各自的 TableMaker
            std::jthread sim_worker([&]() {
                while (true) {
                    auto ev = ts_source.next_event();
                    if (!ev)
                        break;
                    _simulation_table_maker.write_event(*ev);
                }
            });

            std::jthread rec_worker([&]() {
                while (true) {
                    auto ev = ts_source.next_event();
                    if (!ev)
                        break;
                    _reconstructor_table_maker.write_event(*ev);
                }
            });

            std::jthread tel_worker([&]() {
                while (true) {
                    auto ev = ts_source.next_event();
                    if (!ev)
                        break;
                    _telescope_table_maker.write_event(*ev);
                }
            });

            // 必须等待三个 worker 完成（保证 builders 不再被写）
            sim_worker.join();
            rec_worker.join();
            tel_worker.join();

            // 然后把每个 TableMaker 做成 arrow::Table 并返回
            auto simulation_table = _simulation_table_maker.make_table();
            auto reconstructor_table = _reconstructor_table_maker.make_table();
            auto telescope_table = _telescope_table_maker.make_table();

            return DataTable{simulation_table, reconstructor_table,
                             telescope_table};
        }

    private:
        EventSource &_source;
        TableMaker<SimulationData> _simulation_table_maker;
        TableMaker<ReconstructorData> _reconstructor_table_maker;
        TableMaker<TelescopeData> _telescope_table_maker;
    };
} // namespace df
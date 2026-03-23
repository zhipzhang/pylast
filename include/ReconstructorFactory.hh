#pragma once


#include "GeometryReconstructor.hh"

class ReconstructorFactory
{
public:
    static ReconstructorFactory& instance() {
        static ReconstructorFactory factory;
        return factory;
    }
    std::unique_ptr<Reconstructor> create(const std::string& type, const SubarrayDescription& subarray, const json& config);
    void register_reconstructor(const std::string& type, const std::function<std::unique_ptr<Reconstructor>(const SubarrayDescription& subarray, const json& config)>& creator);
    bool is_registered(const std::string& type) const;
private:
    std::map<std::string, std::function<std::unique_ptr<Reconstructor>(const SubarrayDescription& subarray, const json& config)>> creators_;
};

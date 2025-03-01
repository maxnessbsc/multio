/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

// @author Adam Warde

#include <unistd.h>
#include <cstring>
#include <limits>

#include "multio/api/multio_c.h"
#include "multio/api/multio_c_cpp_utils.h"
#include "multio/tools/MultioTool.h"
#include "multio/util/ConfigurationPath.h"

#include "eckit/io/FileHandle.h"
#include "eckit/log/Log.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"

using multio::util::configuration_file_name;
using multio::util::configuration_path_name;

class MultioReplayExampleCApi final : public multio::MultioTool {
public:
    MultioReplayExampleCApi(int argc, char** argv);

private:
    void usage(const std::string& tool) const override {
        eckit::Log::info() << std::endl << "Usage: " << tool << " [options]" << std::endl;
    }

    void init(const eckit::option::CmdArgs& args) override;

    void finish(const eckit::option::CmdArgs& args) override;

    void execute(const eckit::option::CmdArgs& args) override;

    void initClient();

    eckit::Buffer readFields();
    void writeFields(const eckit::Buffer& data);

    std::string transportType_ = "mpi";
    std::string pathtodata_;

    int step_ = 1;

    multio_handle_t* multio_handle = nullptr;
};

MultioReplayExampleCApi::MultioReplayExampleCApi(int argc, char** argv) : multio::MultioTool(argc, argv) {
    options_.push_back(new eckit::option::SimpleOption<std::string>("transport", "Type of transport layer"));
    options_.push_back(new eckit::option::SimpleOption<std::string>("path", "Path to data"));
    options_.push_back(new eckit::option::SimpleOption<long>("step", "Time counter for the field to replay"));
}

void MultioReplayExampleCApi::init(const eckit::option::CmdArgs& args) {
    args.get("transport", transportType_);
    args.get("path", pathtodata_);
    args.get("step", step_);

    initClient();
}

void MultioReplayExampleCApi::finish(const eckit::option::CmdArgs&) {
    multio_delete_handle(multio_handle);
}

void MultioReplayExampleCApi::execute(const eckit::option::CmdArgs&) {
    multio_open_connections(multio_handle);

    eckit::Buffer data = readFields();
    writeFields(data);

    multio_close_connections(multio_handle);
}

void MultioReplayExampleCApi::initClient() {
    multio_configuration_t* multio_cc = nullptr;
    multio_new_configuration_from_filename(&multio_cc, configuration_file_name().localPath());
    multio_new_handle(&multio_handle, multio_cc);
    multio_delete_configuration(multio_cc);
}

eckit::Buffer MultioReplayExampleCApi::readFields() {
    const char* conf_path = pathtodata_.c_str();

    auto field = eckit::PathName{conf_path};

    eckit::FileHandle infile{field.fullName()};
    size_t bytes = infile.openForRead();
    eckit::Length len = field.size();
    eckit::Buffer buffer(len);
    {
        eckit::AutoClose closer(infile);
        infile.read(buffer.data(), len);
    }

    return buffer;
}

void MultioReplayExampleCApi::writeFields(const eckit::Buffer& data) {

    multio_metadata_t* md = nullptr;

    multio_new_metadata(&md);

    const char* key = "test_int";
    int value = 1;
    multio_metadata_set_int(md, key, value);

    key = "test_long";
    long long_value = 1;

    multio_metadata_set_long(md, key, long_value);

    key = "test_long_long";
    long long ll_value = 1;
    multio_metadata_set_longlong(md, key, ll_value);

    key = "test_string";
    const char* s_value = "test_val";
    multio_metadata_set_string(md, key, s_value);

    multio_write_field(multio_handle, md, reinterpret_cast<const double*>(data.data()), data.size());

    multio_delete_metadata(md);
}

int main(int argc, char** argv) {
    MultioReplayExampleCApi tool(argc, argv);
    return tool.start();
}

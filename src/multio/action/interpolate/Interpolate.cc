/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "multio/action/interpolate/Interpolate.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>

#include "eckit/exception/Exceptions.h"
#include "eckit/parser/YAMLParser.h"

#include "metkit/mars/MarsLanguage.h"

#include "mir/api/MIRJob.h"
#include "mir/input/RawInput.h"
#include "mir/output/ResizableOutput.h"
#include "mir/param/SimpleParametrisation.h"
#include "mir/repres/gauss/reduced/Reduced.h"

#include "multio/LibMultio.h"
#include "multio/util/PrecisionTag.h"


namespace multio::action::interpolate {


void fill_input(const eckit::LocalConfiguration& cfg, mir::param::SimpleParametrisation& param) {
    ASSERT(cfg.has("input"));
    const auto input = cfg.getString("input");

    static const std::regex sh("(T|TCO|TL)([1-9][0-9]*)");
    static const std::regex gg("([FNO])([1-9][0-9]*)");

    std::smatch match;
    if (std::regex_match(input, match, sh)) {
        param.set("spectral", true);
        param.set("truncation", std::stol(match[2].str()));
        return;
    }

    if (std::regex_match(input, match, gg)) {
        param.set("gridded", true);
        param.set("gridType", match[1].str() == "F" ? "regular_gg" : "reduced_gg");
        param.set("N", std::stol(match[2].str()));
        param.set("north", 90.);
        param.set("west", 0.);
        param.set("south", -90.);
        param.set("east", 360.);

        if (match[1].str() != "F") {
            param.set("pl", mir::repres::gauss::reduced::Reduced::pls(match[0].str()));
        }
        return;
    }

    NOTIMP;
}


void fill_job(const eckit::LocalConfiguration& cfg, mir::param::SimpleParametrisation& destination) {
    static const struct PostProcKeys : std::vector<std::string> {
        PostProcKeys() {
            const auto yaml = eckit::YAMLParser::decodeFile(metkit::mars::MarsLanguage::languageYamlFile());
            for (const auto& key : yaml["_postproc"].keys().as<eckit::ValueList>()) {
                emplace_back(key.as<std::string>());
            }
        }
        bool contains(const std::string& key) const { return std::find(begin(), end(), key) != end(); }
    } postproc;

    ASSERT(not postproc.contains("input"));
    ASSERT(not postproc.contains("options"));

    auto set = [&destination, &cfg](const std::string& key, const eckit::Value& value) {
        if (value.isList()) {
            value.head().isDouble()   ? destination.set(key, cfg.getDoubleVector(key))
            : value.head().isNumber() ? destination.set(key, cfg.getLongVector(key))
            : value.head().isString() ? destination.set(key, cfg.getStringVector(key))
                                      : NOTIMP;
            return;
        }

        value.isBool()     ? destination.set(key, cfg.getBool(key))
        : value.isDouble() ? destination.set(key, cfg.getDouble(key))
        : value.isNumber() ? destination.set(key, cfg.getInt(key))
        : value.isString() ? destination.set(key, cfg.getString(key).c_str())
                           : NOTIMP;
    };

    for (const auto& key : postproc) {
        if (cfg.has(key)) {
            set(key, cfg.getSubConfiguration(key).get());
        }
    }

    if (cfg.has("options")) {
        const auto& options = cfg.getSubConfiguration("options");
        for (const auto& key : options.keys()) {
            set(key, cfg.getSubConfiguration(key).get());
        }
    }
};


template <typename A, typename B>
message::Message convert_precision(message::Message&& msg) {
    const size_t N = msg.payload().size() / sizeof(A);
    eckit::Buffer buffer(N * sizeof(B));

    auto md = msg.metadata();
    md.set("globalSize", buffer.size());
    md.set("precision", std::is_same<B, double>::value ? "double" : std::is_same<B, float>::value ? "single" : NOTIMP);

    const auto* a = reinterpret_cast<const A*>(msg.payload().data());
    auto* b = reinterpret_cast<B*>(buffer.data());
    for (size_t i = 0; i < N; ++i) {
        *(b++) = static_cast<B>(*(a++));
    }

    return {message::Message::Header{msg.tag(), msg.source(), msg.destination(), std::move(md)}, std::move(buffer)};
}


message::Message Interpolate::InterpolateInSinglePrecision(message::Message&& msg) const {
    // convert single/double precision, interpolate, convert double/single
    return convert_precision<double, float>(
        InterpolateInDoublePrecision(convert_precision<float, double>(std::move(msg))));
}


message::Message Interpolate::InterpolateInDoublePrecision(message::Message&& msg) const {
    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: Metadata of the input message :: " << std::endl
                             << msg.metadata() << std::endl
                             << std::endl;

    const auto& config = Action::confCtx_.config();

    const double* data = reinterpret_cast<const double*>(msg.payload().data());
    const size_t size = msg.payload().size() / sizeof(double);

    mir::param::SimpleParametrisation inputPar;
    fill_input(config, inputPar);

    mir::input::RawInput input(data, size, inputPar);

    mir::api::MIRJob job;
    fill_job(config, job);

    if (msg.metadata().has("missingValue")) {
        job.set("missing_value", msg.metadata().getDouble("missingValue"));
    }

    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: input :: " << std::endl << inputPar << std::endl << std::endl;

    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: job " << std::endl << job << std::endl << std::endl;

    std::vector<double> outData;
    mir::param::SimpleParametrisation outMetadata;
    mir::output::ResizableOutput output(outData, outMetadata);

    job.execute(input, output);

    message::Metadata md;
    md.set("globalSize", outData.size());
    md.set("precision", "double");

    eckit::Buffer buffer(reinterpret_cast<const char*>(outData.data()), outData.size() * sizeof(double));

    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: Metadata of the output message :: " << std::endl
                             << md << std::endl
                             << std::endl;

    return {message::Message::Header{message::Message::Tag::Field, msg.source(), msg.destination(), std::move(md)},
            std::move(buffer)};
}


void Interpolate::executeImpl(message::Message msg) const {
    switch (msg.tag()) {
        case (message::Message::Tag::Field): {
            executeNext(util::dispatchPrecisionTag(msg.precision(), [&](auto pt) -> message::Message {
                using PT = typename decltype(pt)::type;
                if (std::is_same<double, PT>::value) {
                    return InterpolateInDoublePrecision(std::move(msg));
                }
                else if (std::is_same<float, PT>::value) {
                    return InterpolateInSinglePrecision(std::move(msg));
                }
                NOTIMP;
            }));
            break;
        };
        case (message::Message::Tag::StepComplete): {
            executeNext(msg);
            break;
        }
        default: {
            throw eckit::BadValue("Action::Interpolate :: Unsupported message tag", Here());
            break;
        }
    };
}


void Interpolate::print(std::ostream& os) const {
    os << "Interpolate";
}


static ActionBuilder<Interpolate> InterpolateBuilder("interpolate");


}  // namespace multio::action::interpolate

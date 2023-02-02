/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Domokos Sarmany
/// @author Simon Smart
/// @author Tiago Quintino

/// @date Jan 2019

#pragma once

#include <iosfwd>
#include <unordered_map>

#include "multio/action/ChainedAction.h"
#include "multio/domain/Mappings.h"
#include "multio/util/PrecisionTag.h"

namespace multio {
namespace action {

using message::Message;

class MessageMap : private std::map<std::string, Message> {
public:
    using std::map<std::string, Message>::at;

    bool contains(const std::string& key) const {
        return find(key) != end() && processedParts_.find(key) != std::end(processedParts_);
    }

    std::size_t partsCount(const std::string& key) const {
        ASSERT(contains(key));
        return processedParts_.at(key).size();
    }

    void bookProcessedPart(const std::string& key, message::Peer peer) {
        ASSERT(contains(key));
        auto ret = processedParts_.at(key).insert(std::move(peer));
        if (not ret.second) {
            eckit::Log::warning() << " Field " << key << " has been aggregated already" << std::endl;
        }
    }

    void addNew(const Message& msg) {
        ASSERT(not contains(msg.fieldId()));
        multio::util::dispatchPrecisionTag(msg.precision(), [&](auto pt) {
            using PT = typename decltype(pt)::type;
            emplace(msg.fieldId(),
                    Message{Message::Header{msg.header()}, eckit::Buffer{msg.globalSize() * sizeof(PT)}});
            processedParts_.emplace(msg.fieldId(), std::set<message::Peer>{});
        });
    }

    Message extract(const std::string& fid) {

        auto it = find(fid);
        ASSERT(it != end());

        auto msgOut = std::move(it->second);

        processedParts_.erase(it->first);
        erase(it);

        return msgOut;
    }

    void print(std::ostream& os) {
        os << "Aggregate(for " << size() << " fields = [";
        for (const auto& mp : *this) {
            auto const& domainMap = domain::Mappings::instance().get(mp.second.domain());
            os << '\n'
               << "  --->  " << mp.first << " ---> Aggregated " << partsCount(mp.first) << " parts of a total of "
               << (domainMap.isComplete() ? domainMap.size() : 0);
        }
        os << "])";
    }

private:
    std::map<std::string, std::set<message::Peer>> processedParts_;
};


class Aggregate : public ChainedAction {
public:
    explicit Aggregate(const ConfigurationContext& confCtx);

    void executeImpl(Message msg) const override;

private:
    void print(std::ostream& os) const override;

    bool handleField(const Message& msg) const;
    bool handleFlush(const Message& msg) const;

    Message createGlobalField(const std::string& msg) const;
    bool allPartsArrived(const Message& msg) const;

    auto flushCount(const Message& msg) const;

    mutable MessageMap msgMap_;
    mutable std::map<std::string, unsigned int> flushes_;
};

}  // namespace action
}  // namespace multio

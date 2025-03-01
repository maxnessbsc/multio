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

#include <unordered_map>

#include "multio/action/ChainedAction.h"
#include "multio/message/MetadataMapping.h"

namespace multio::action {

class MetadataMapping : public ChainedAction {
public:
    explicit MetadataMapping(const ComponentConfiguration& compConf);

    void executeImpl(message::Message msg) override;

protected:
    void applyInplace(message::Metadata& msg) const;
    message::Metadata apply(const message::Metadata& msg) const;
    message::Metadata apply(message::Metadata&& msg) const;

private:
    void print(std::ostream& os) const override;

    std::string name_;
    const std::vector<message::MetadataMapping>& mappings_;
    message::MetadataMappingOptions options_;
};

}  // namespace multio::action

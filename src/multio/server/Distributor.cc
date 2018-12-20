
#include "Distributor.h"

#include "eckit/io/Buffer.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/utils/RendezvousHash.h"
#include "eckit/value/Value.h"

#include "atlas/field/Field.h"
#include "atlas/util/Metadata.h"

#include "multio/server/Message.h"
#include "multio/server/msg_tag.h"
#include "multio/server/print_buffer.h"
#include "multio/server/SerialisationHelpers.h"

namespace multio {
namespace server {

namespace {
const bool use_md5 = true;
auto hash_val(const std::string& str) -> size_t {
    return use_md5 ? std::hash<std::string>{}(eckit::RendezvousHash::md5(str))
                   : std::hash<std::string>{}(str);
}
}  // namespace

Distributor::Distributor(const Transport& trans) : transport_(trans) {}

size_t Distributor::computeHash(const atlas::Field& field) const {
    auto meta_str = pack_metadata(field.metadata());
    return (hash_val(meta_str) % transport_.noServers() + transport_.noClients());
}

void Distributor::sendLocalPlan(const atlas::Field& field) const {
    auto field_type = field.metadata().get<std::string>("field_type");
    if (distributed_plans.find(field_type) != distributed_plans.end()) {
        return;
    }

    // Register sending this plan
    auto plan = fetch_local_plan(field.metadata(), transport_.noClients(), transport_.clientRank());

    Message msg(0, -1, msg_tag::plan_data);
    local_plan_to_message(plan, msg);

    // TODO: create a sendToAllServers member function on the transport_. We can then get rid of
    // that awkward setter on the message class
    for (auto ii = 0u; ii != transport_.noServers(); ++ii) {
        msg.peer(static_cast<int>(transport_.noClients() + ii));
        transport_.send(msg);
    }

    auto counter = 0u;
    do {
        Message msg(0, -1, msg_tag::plan_complete);
        transport_.receive(msg);
        eckit::Buffer buffer{msg.size()};
        msg.read(buffer, msg.size());
        std::string str_buf(buffer);
        str_buf.resize(buffer.size());
        if (str_buf == plan.name()) {
            ++counter;
        }
    } while (counter != transport_.noServers());

    // Register sending this plan
    distributed_plans[field_type] = plan;
}

void Distributor::sendField(const atlas::Field& field) const {
    sendLocalPlan(field);

    auto server_rank = computeHash(field);

    Message msg(0, static_cast<int>(server_rank), msg_tag::field_data);
    atlas_field_to_message(field, msg);

    transport_.send(msg);
}

void Distributor::sendForecastComplete() const {
    for (auto ii = 0u; ii != transport_.noServers(); ++ii) {
        Message msg(0, static_cast<int>(transport_.noClients() + ii), msg_tag::forecast_complete);
        transport_.send(msg);
    }
}

// Private members

void Distributor::print(std::ostream& os) const {
    os << "Distributor initialised with " << transport_;
}

}  // namespace server
}  // namespace multio

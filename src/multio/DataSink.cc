/*
 * (C) Copyright 1996-2015 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Tiago Quintino
/// @author Simon Smart
/// @date Dec 2015

#include "multio/DataSink.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/parser/JSON.h"
#include "eckit/thread/AutoLock.h"
#include "eckit/thread/Mutex.h"

using namespace eckit;

namespace multio {

//----------------------------------------------------------------------------------------------------------------------

static Mutex *local_mutex = 0;
static std::map<std::string, DataSinkFactory*> *m = 0;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static void init() {
    local_mutex = new Mutex();
    m = new std::map<std::string, DataSinkFactory*>();
}

DataSinkFactory::DataSinkFactory(const std::string &name) :
    name_(name) {

    pthread_once(&once, init);

    AutoLock<Mutex> lock(local_mutex);

    ASSERT(m->find(name) == m->end());
    (*m)[name] = this;
}


DataSinkFactory::~DataSinkFactory() {
    AutoLock<Mutex> lock(local_mutex);
    m->erase(name_);
}


void DataSinkFactory::list(std::ostream& out) {

    pthread_once(&once, init);

    AutoLock<Mutex> lock(local_mutex);

    const char* sep = "";
    for (std::map<std::string, DataSinkFactory*>::const_iterator j = m->begin() ; j != m->end() ; ++j) {
        out << sep << (*j).first;
        sep = ", ";
    }
}


DataSink* DataSinkFactory::build(const std::string &name, const Configuration& config) {

    pthread_once(&once, init);

    AutoLock<Mutex> lock(local_mutex);

    Log::info() << "Looking for DataSinkFactory [" << name << "]" << std::endl;

    std::map<std::string, DataSinkFactory *>::const_iterator j = m->find(name);
    if (j == m->end()) {
        Log::error() << "No DataSinkFactory for [" << name << "]" << std::endl;
        Log::error() << "DataSinkFactories are:" << std::endl;
        for (j = m->begin() ; j != m->end() ; ++j)
            Log::error() << "   " << (*j).first << std::endl;
        throw SeriousBug(std::string("No DataSinkFactory called ") + name);
    }

    return (*j).second->make(config);
}

//----------------------------------------------------------------------------------------------------------------------

DataSink::DataSink(const Configuration& config) :
    failOnError_( config.getBool("failOnError",true) ),
    journaled_( config.getBool("journaled",false) ),
    journalAlways_( config.getBool("journalAlways", false) ),
    config_(config.get()),
    id_(-1) {
}

DataSink::~DataSink() {
}

bool DataSink::ready() const
{
    return true; // default for synchronous sinks
}


std::string DataSink::json() const {

    std::stringstream json_stream;
    JSON config_json(json_stream);
    config_json << config_.get();

    return json_stream.str();
}


void DataSink::setId(int id) {
    id_ = id;
}

//----------------------------------------------------------------------------------------------------------------------

int DataSink::iopenfdb(const char *name, const char *mode, int name_len, int mode_len) {
    return 0;
}

int DataSink::iclosefdb() {
    return 0;
}

int DataSink::iinitfdb() {
    return 0;
}

int DataSink::isetcommfdb(int *rank) {
    return 0;
}

int DataSink::isetrankfdb(int *rank) {
    return 0;
}

int DataSink::iset_fdb_root(const char *name, int name_len) {
    return 0;
}

int DataSink::iflushfdb() {
    return 0;
}

int DataSink::isetfieldcountfdb(int *all_ranks, int *this_rank) {
    return 0;
}

int DataSink::isetvalfdb(const char *name, const char *value, int name_len, int value_len) {
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace multio


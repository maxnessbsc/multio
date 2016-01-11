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
/// @date Dec 2015


#ifndef multio_MultIO_H
#define multio_MultIO_H

#include <iosfwd>
#include <string>
#include <vector>

#include "eckit/memory/NonCopyable.h"
#include "eckit/io/Length.h"

#include "multio/DataSink.h"
#include "multio/JournalRecord.h"

namespace multio {

//----------------------------------------------------------------------------------------------------------------------

class MultIO : public DataSink {

public:

    MultIO(const eckit::Configuration& config);

    virtual ~MultIO();

    virtual bool ready() const;

    virtual void write(const void* buffer,
                       const eckit::Length& length,
                       JournalRecord *const record = NULL,
                       Metadata *const Metadata = NULL);

    virtual void replayRecord(const JournalRecord& record);

    void commitJournal();

    ///
    /// LEGACY INTERFACE TO REMOVE AFTER IFS CHANGED TO SIMPLE WRITE() INTERFACE
    ///

    virtual int iopenfdb(const char *name, const char *mode, int name_len, int mode_len);
    virtual int iinitfdb();
    virtual int iclosefdb();

    virtual int isetcommfdb(int *rank);
    virtual int isetrankfdb(int *rank);
    virtual int iset_fdb_root(const char *name, int name_len);

    // virtual int ireadfdb(void *data, int *words);
    // virtual int iwritefdb(void *data, int *words);
    virtual int iflushfdb();

    virtual int isetfieldcountfdb(int *all_ranks, int *this_rank);
    virtual int isetvalfdb(const char *name, const char *value, int name_len, int value_len);

protected: // types

    typedef std::vector<DataSink*> sink_store_t;

protected:

    virtual void print(std::ostream&) const;

protected: // members

    Journal journal_;

    sink_store_t sinks_;

private: // methods

    friend std::ostream &operator<<(std::ostream &s, const MultIO &p) {
        p.print(s);
        return s;
    }

};

//----------------------------------------------------------------------------------------------------------------------

}  // namespace multio

#endif


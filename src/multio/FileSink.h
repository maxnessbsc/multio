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

#ifndef multio_FileSink_H
#define multio_FileSink_H

#include <iosfwd>
#include <string>
#include <vector>

#include "eckit/io/Length.h"
#include "eckit/memory/NonCopyable.h"
#include "eckit/memory/ScopedPtr.h"
#include "eckit/thread/Mutex.h"
#include "eckit/filesystem/PathName.h"
#include "multio/DataSink.h"
#include "multio/JournalRecord.h"

//----------------------------------------------------------------------------------------------------------------------

namespace eckit { class FileHandle; }

namespace multio {

class FileSink : public DataSink {

public:

    FileSink(const eckit::Configuration& config);

    virtual ~FileSink();

private: // methods

    virtual void write(eckit::DataBlobPtr blob);

    virtual void print(std::ostream&) const;

private: // members

    eckit::PathName path_;
    eckit::ScopedPtr<eckit::DataHandle> handle_;
    eckit::Mutex mutex_;
};

//----------------------------------------------------------------------------------------------------------------------

}  // namespace multio

#endif // multio_FileSink_H


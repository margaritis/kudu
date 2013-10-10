// Copyright (c) 2013, Cloudera, inc.

#include "common/schema.h"
#include "gutil/stringprintf.h"
#include "gutil/strings/join.h"
#include "gutil/strings/strcat.h"
#include "gutil/strings/substitute.h"
#include "util/status.h"

namespace kudu {

string ColumnSchema::ToString() const {
  return strings::Substitute("$0[type='$1' $2]",
                             name_,
                             type_info_->name(),
                             is_nullable_ ? "NULLABLE" : "NOT NULL");
}

Status Schema::Reset(const vector<ColumnSchema>& cols,
                     const vector<size_t>& ids,
                     int key_columns) {
  cols_ = cols;
  num_key_columns_ = key_columns;

  if (PREDICT_FALSE(key_columns > cols_.size())) {
    return Status::InvalidArgument(
      "Bad schema", "More key columns than columns");
  }

  if (PREDICT_FALSE(!ids.empty() && ids.size() != cols_.size())) {
    return Status::InvalidArgument("Bad schema",
      "The number of ids does not match with the number of columns");
  }

  // Verify that the key columns are not nullable
  for (int i = 0; i < key_columns; ++i) {
    if (PREDICT_FALSE(cols_[i].is_nullable())) {
      return Status::InvalidArgument(
        "Bad schema", "Nullable key columns not supported");
    }
  }

  // Calculate the offset of each column in the row format.
  col_offsets_.reserve(cols_.size() + 1);  // Include space for total byte size at the end.
  size_t off = 0;
  size_t i = 0;
  BOOST_FOREACH(const ColumnSchema &col, cols) {
    name_to_index_[col.name()] = i++;
    col_offsets_.push_back(off);
    off += col.type_info().size();
  }

  if (PREDICT_FALSE(cols.size() != name_to_index_.size())) {
    return Status::InvalidArgument(
      "Bad schema", "Duplicate name present in schema!");
  }

  // Add an extra element on the end for the total
  // byte size
  col_offsets_.push_back(off);

  // Initialize IDs mapping
  col_ids_ = ids;
  id_to_index_.clear();
  for (int i = 0; i < ids.size(); ++i) {
    id_to_index_[col_ids_[i]] = i;
  }

  return Status::OK();
}

string Schema::ToString() const {
  vector<string> col_strs;
  BOOST_FOREACH(const ColumnSchema &col, cols_) {
    col_strs.push_back(col.ToString());
  }

  return StrCat("Schema [",
                JoinStrings(col_strs, ", "),
                "]");
}

} // namespace kudu

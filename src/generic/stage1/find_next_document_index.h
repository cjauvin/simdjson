#ifndef SIMDJSON_SRC_GENERIC_STAGE1_FIND_NEXT_DOCUMENT_INDEX_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_FIND_NEXT_DOCUMENT_INDEX_H
#include <generic/stage1/base.h>
#include <simdjson/generic/dom_parser_implementation.h>
#include <cstring>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

simdjson_inline bool is_json_whitespace(uint8_t c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

simdjson_inline uint32_t internal_find_next_document_index_comma_separated(dom_parser_implementation &parser, stage1_mode partial, uint32_t base);

/**
  * This algorithm is used to quickly identify the last structural position that
  * makes up a complete document.
  *
  * It does this by going backwards and finding the last *document boundary* (a
  * place where one value follows another without a comma between them). If the
  * last document (the characters after the boundary) has an equal number of
  * start and end brackets, it is considered complete.
  *
  * Simply put, we iterate over the structural characters, starting from
  * the end. We consider that we found the end of a JSON document when the
  * first element of the pair is NOT one of these characters: '{' '[' ':' ','
  * and when the second element is NOT one of these characters: '}' ']' ':' ','.
  *
  * This simple comparison works most of the time, but it does not cover cases
  * where the batch's structural indexes contain a perfect amount of documents.
  * In such a case, we do not have access to the structural index which follows
  * the last document, therefore, we do not have access to the second element in
  * the pair, and that means we cannot identify the last document. To fix this
  * issue, we keep a count of the open and closed curly/square braces we found
  * while searching for the pair. When we find a pair AND the count of open and
  * closed curly/square braces is the same, we know that we just passed a
  * complete document, therefore the last json buffer location is the end of the
  * batch.
  *
  * In the case that allow_comma_separated is true, if no regular boundary, we
  * will try another algorithm, slightly costlier, which looks for the last
  * comma found at depth 0.
  */
simdjson_inline uint32_t find_next_document_index(dom_parser_implementation &parser, stage1_mode partial, bool allow_comma_separated) {
  // Variant: do not count separately, just figure out depth
  uint32_t base = [&parser]() -> uint32_t {
    if(parser.n_structural_indexes == 0) { return 0; }
    auto arr_cnt = 0;
    auto obj_cnt = 0;
    for (auto i = parser.n_structural_indexes - 1; i > 0; i--) {
      auto idxb = parser.structural_indexes[i];
      switch (parser.buf[idxb]) {
      case ':':
      case ',':
        continue;
      case '}':
        obj_cnt--;
        continue;
      case ']':
        arr_cnt--;
        continue;
      case '{':
        obj_cnt++;
        break;
      case '[':
        arr_cnt++;
        break;
      }
      auto idxa = parser.structural_indexes[i - 1];
      switch (parser.buf[idxa]) {
      case '{':
      case '[':
      case ':':
      case ',':
        continue;
      }
      // Last document is complete, so the next document will appear after!
      if (!arr_cnt && !obj_cnt) {
        return parser.n_structural_indexes;
      }
      // Last document is incomplete; mark the document at i + 1 as the next one
      return i;
    }
    // If we made it to the end, we want to finish counting to see if we have a full document.
    switch (parser.buf[parser.structural_indexes[0]]) {
      case '}':
        obj_cnt--;
        break;
      case ']':
        arr_cnt--;
        break;
      case '{':
        obj_cnt++;
        break;
      case '[':
        arr_cnt++;
        break;
    }
    if (!arr_cnt && !obj_cnt) {
      // We have a complete document.
      return parser.n_structural_indexes;
    }
    return 0;
  }();

  if (allow_comma_separated) {
    return internal_find_next_document_index_comma_separated(parser, partial, base);
  }
  return base;
}

/**
 * Fallback for find_next_document_index that treats a top-level comma as a document boundary.
 *
 * It finds the last comma at depth 0, then checks whether the document that starts after it is
 * complete in this batch. If complete, we keep the full batch; if incomplete, we stop before
 * that trailing document so it can be reprocessed once more data arrives.
 */
simdjson_inline uint32_t internal_find_next_document_index_comma_separated(dom_parser_implementation &parser, stage1_mode partial, uint32_t base) {

  int32_t depth = 0;
  int64_t last_comma_index = -1;
  for (uint32_t i = 0; i < parser.n_structural_indexes; i++) {
    uint32_t idx = parser.structural_indexes[i];
    switch (parser.buf[idx]) {
      case '{':
      case '[':
        depth++;
        break;
      case '}':
      case ']':
        depth--;
        break;
      case ',':
        if (depth == 0) {
          last_comma_index = i;
        }
        break;
      default:
        break;
    }
    if (depth < 0) {
      return base;
    }
  }

  if (last_comma_index < 0) {
    return base;
  }
  uint32_t next_index = uint32_t(last_comma_index + 1);
  if (next_index >= parser.n_structural_indexes) {
    return parser.n_structural_indexes;
  }

  uint32_t start_pos = parser.structural_indexes[next_index];
  uint8_t start_char = parser.buf[start_pos];
  bool complete = false;
  if (start_char == '{' || start_char == '[') {
    complete = (depth == 0);
  } else if (start_char == '"') {
    bool escape = false;
    for (size_t i = start_pos + 1; i < parser.len; i++) {
      uint8_t c = parser.buf[i];
      if (escape) {
        escape = false;
        continue;
      }
      if (c == '\\') {
        escape = true;
        continue;
      }
      if (c == '"') {
        complete = true;
        break;
      }
    }
  // Check for literals: true, false, null
  } else if (start_char == 't') {
    if (start_pos + 4 <= parser.len && std::memcmp(parser.buf + start_pos, "true", 4) == 0) {
      if (start_pos + 4 == parser.len) {
        complete = (partial == stage1_mode::streaming_final);
      } else {
        complete = is_json_whitespace(parser.buf[start_pos + 4]) || parser.buf[start_pos + 4] == ',' || parser.buf[start_pos + 4] == ']' || parser.buf[start_pos + 4] == '}';
      }
    }
  } else if (start_char == 'f') {
    if (start_pos + 5 <= parser.len && std::memcmp(parser.buf + start_pos, "false", 5) == 0) {
      if (start_pos + 5 == parser.len) {
        complete = (partial == stage1_mode::streaming_final);
      } else {
        complete = is_json_whitespace(parser.buf[start_pos + 5]) || parser.buf[start_pos + 5] == ',' || parser.buf[start_pos + 5] == ']' || parser.buf[start_pos + 5] == '}';
      }
    }
  } else if (start_char == 'n') {
    if (start_pos + 4 <= parser.len && std::memcmp(parser.buf + start_pos, "null", 4) == 0) {
      if (start_pos + 4 == parser.len) {
        complete = (partial == stage1_mode::streaming_final);
      } else {
        complete = is_json_whitespace(parser.buf[start_pos + 4]) || parser.buf[start_pos + 4] == ',' || parser.buf[start_pos + 4] == ']' || parser.buf[start_pos + 4] == '}';
      }
    }
  } else {
    for (size_t i = start_pos; i < parser.len; i++) {
      uint8_t c = parser.buf[i];
      if (is_json_whitespace(c) || c == ',' || c == ']' || c == '}') {
        complete = true;
        break;
      }
    }
    if (!complete && partial == stage1_mode::streaming_final) {
      complete = true;
    }
  }

  if (complete) {
    return parser.n_structural_indexes;
  }
  return next_index;
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_FIND_NEXT_DOCUMENT_INDEX_H

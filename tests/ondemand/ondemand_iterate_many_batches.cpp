#include "simdjson.h"
#include "test_ondemand.h"

#include <cstdint>
#include <string>

using namespace simdjson;
using namespace std;

namespace iterate_many_batches_tests {

bool comma_small_batch() {
  TEST_START();
  auto json = R"({"foo": 1},{"foo": 2},{"foo": 3})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 15, true).get(doc_stream));

  size_t doc_count = 0;
  int64_t expected_values[] = {1, 2, 3};
  for (auto doc : doc_stream)
  {
    int64_t val;
    ASSERT_SUCCESS(doc["foo"].get(val));
    ASSERT_EQUAL(val, expected_values[doc_count]);
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 3);
  TEST_SUCCEED();
}

bool comma_tiny_batch() {
  TEST_START();
  auto json = R"({"a":1},{"a":2},{"a":3},{"a":4})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 10, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    int64_t val;
    ASSERT_SUCCESS(doc["a"].get(val));
    ASSERT_EQUAL(val, static_cast<int64_t>(doc_count + 1));
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 4);
  TEST_SUCCEED();
}

bool comma_nested_objects() {
  TEST_START();
  auto json = R"({"x":1,"y":2},{"a":[1,2,3]},{"b":{"c":4}})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 20, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 3);
  TEST_SUCCEED();
}

bool comma_deeply_nested() {
  TEST_START();
  auto json = R"({"a":{"b":{"c":{"d":1}}}},{"x":{"y":{"z":2}}})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 20, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 2);
  TEST_SUCCEED();
}

bool comma_mixed_types() {
  TEST_START();
  auto json = R"({"obj":1},[1,2,3],123,"string",true,null)"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 15, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto type = doc.type();
    ASSERT_SUCCESS(type.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 6);
  TEST_SUCCEED();
}

bool whitespace_separated_small_batch() {
  TEST_START();
  auto json = R"({"foo": 1} {"foo": 2} {"foo": 3})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 15).get(doc_stream));

  size_t doc_count = 0;
  int64_t expected_values[] = {1, 2, 3};
  for (auto doc : doc_stream)
  {
    int64_t val;
    ASSERT_SUCCESS(doc["foo"].get(val));
    ASSERT_EQUAL(val, expected_values[doc_count]);
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 3);
  TEST_SUCCEED();
}

bool comma_at_batch_boundary() {
  TEST_START();
  auto json = R"({"x":1},{"y":2})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 12, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 2);
  TEST_SUCCEED();
}

bool incomplete_document_at_boundary() {
  TEST_START();
  auto json = R"({"id": 1} {"id": 2, "data": [1, 2, 3]} {"id": 3})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 35).get(doc_stream));

  size_t doc_count = 0;
  int64_t expected_ids[] = {1, 2, 3};
  for (auto doc : doc_stream)
  {
    int64_t val;
    ASSERT_SUCCESS(doc["id"].get(val));
    ASSERT_EQUAL(val, expected_ids[doc_count]);
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 3);
  TEST_SUCCEED();
}

bool single_document() {
  TEST_START();
  auto json = R"({"single": "document"})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 10, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    std::string_view val;
    ASSERT_SUCCESS(doc["single"].get(val));
    ASSERT_EQUAL(val, "document");
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 1);
  TEST_SUCCEED();
}

bool empty_input() {
  TEST_START();
  auto json = R"(   )"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 10, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    (void)doc;
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 0);
  TEST_SUCCEED();
}

bool comma_large_batch() {
  TEST_START();
  auto json = R"({"foo": 1},{"foo": 2},{"foo": 3})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 1000, true).get(doc_stream));

  size_t doc_count = 0;
  int64_t expected_values[] = {1, 2, 3};
  for (auto doc : doc_stream)
  {
    int64_t val;
    ASSERT_SUCCESS(doc["foo"].get(val));
    ASSERT_EQUAL(val, expected_values[doc_count]);
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 3);
  TEST_SUCCEED();
}

bool comma_arrays() {
  TEST_START();
  auto json = R"([1,2,3],[4,5,6],[7,8,9])"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 12, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto arr = doc.get_array();
    ASSERT_SUCCESS(arr.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 3);
  TEST_SUCCEED();
}

bool comma_nested_arrays() {
  TEST_START();
  auto json = R"([[1,2],[3,4]],[[5,6],[7,8]])"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 15, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto arr = doc.get_array();
    ASSERT_SUCCESS(arr.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 2);
  TEST_SUCCEED();
}

bool comma_many_documents() {
  TEST_START();
  std::string json_str = "{\"i\":0}";
  for (int i = 1; i < 100; i++) {
    json_str += ",{\"i\":" + std::to_string(i) + "}";
  }
  auto json = padded_string(json_str);

  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 50, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    int64_t val;
    ASSERT_SUCCESS(doc["i"].get(val));
    ASSERT_EQUAL(val, static_cast<int64_t>(doc_count));
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 100);
  TEST_SUCCEED();
}

bool mixed_comma_whitespace() {
  TEST_START();
  auto json = R"({"a":1}, {"b":2}  {"c":3},  {"d":4})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 15, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 4);
  TEST_SUCCEED();
}

bool commas_inside_objects() {
  TEST_START();
  auto json = R"({"x":1,"y":2,"z":3},{"a":4,"b":5,"c":6})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 20, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 2);
  TEST_SUCCEED();
}

bool batch_boundary_at_complete_document() {
  TEST_START();
  auto json = R"({"a":1}{"b":2})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 7).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 2);
  TEST_SUCCEED();
}

bool all_documents_incomplete_in_batch() {
  TEST_START();
  auto json = R"({"incomplete": "document"})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 5).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 1);
  TEST_SUCCEED();
}

bool unbalanced_brackets() {
  TEST_START();
  auto json = R"({"a":1},{"b":2)"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 10, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    if (obj.error() == SUCCESS) {
      doc_count++;
    }
  }

  ASSERT_EQUAL(doc_count, 1);
  TEST_SUCCEED();
}

bool incomplete_tail_across_batches() {
  TEST_START();
  auto json = R"({"a":1},{"b":2},{"c":3},{"d":4},{"e":5},{"f":)"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 32, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    ASSERT_SUCCESS(obj.error());
    doc_count++;
  }

  ASSERT_EQUAL(doc_count, 5);
  TEST_SUCCEED();
}

bool incomplete_tail_nested_commas() {
  TEST_START();
  auto json = R"({"a":[1,2,3]},{"b":{"c":4,"d":5,"e":6})"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 32, true).get(doc_stream));

  size_t doc_count = 0;
  for (auto doc : doc_stream)
  {
    auto obj = doc.get_object();
    if (obj.error() == SUCCESS) {
      doc_count++;
    }
  }

  ASSERT_EQUAL(doc_count, 1);
  TEST_SUCCEED();
}

bool run() {
  return comma_small_batch() &&
         comma_tiny_batch() &&
         comma_nested_objects() &&
         comma_deeply_nested() &&
         comma_mixed_types() &&
         whitespace_separated_small_batch() &&
         comma_at_batch_boundary() &&
         incomplete_document_at_boundary() &&
         single_document() &&
         empty_input() &&
         comma_large_batch() &&
         comma_arrays() &&
         comma_nested_arrays() &&
         comma_many_documents() &&
         mixed_comma_whitespace() &&
         commas_inside_objects() &&
         batch_boundary_at_complete_document() &&
         all_documents_incomplete_in_batch() &&
         unbalanced_brackets() &&
         incomplete_tail_across_batches() &&
         incomplete_tail_nested_commas() &&
         true;
}

}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, iterate_many_batches_tests::run);
}

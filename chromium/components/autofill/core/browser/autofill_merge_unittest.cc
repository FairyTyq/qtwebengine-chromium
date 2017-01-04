// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <map>
#include <memory>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/country_names.h"
#include "components/autofill/core/browser/data_driven_test.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/common/form_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if defined(OS_MACOSX)
#include "base/mac/foundation_util.h"
#endif

namespace autofill {

namespace {

const base::FilePath::CharType kTestName[] = FILE_PATH_LITERAL("merge");
const base::FilePath::CharType kFileNamePattern[] = FILE_PATH_LITERAL("*.in");

const char kFieldSeparator[] = ":";
const char kProfileSeparator[] = "---";

const ServerFieldType kProfileFieldTypes[] = {NAME_FIRST,
                                              NAME_MIDDLE,
                                              NAME_LAST,
                                              NAME_FULL,
                                              EMAIL_ADDRESS,
                                              COMPANY_NAME,
                                              ADDRESS_HOME_STREET_ADDRESS,
                                              ADDRESS_HOME_CITY,
                                              ADDRESS_HOME_STATE,
                                              ADDRESS_HOME_ZIP,
                                              ADDRESS_HOME_COUNTRY,
                                              PHONE_HOME_WHOLE_NUMBER};

const base::FilePath& GetTestDataDir() {
  CR_DEFINE_STATIC_LOCAL(base::FilePath, dir, ());
  if (dir.empty()) {
    PathService::Get(base::DIR_SOURCE_ROOT, &dir);
    dir = dir.AppendASCII("components");
    dir = dir.AppendASCII("test");
    dir = dir.AppendASCII("data");
  }
  return dir;
}

const std::vector<base::FilePath> GetTestFiles() {
  base::FilePath dir = GetTestDataDir();
  dir = dir.AppendASCII("autofill").AppendASCII("merge").AppendASCII("input");
  base::FileEnumerator input_files(dir, false, base::FileEnumerator::FILES,
                                   kFileNamePattern);
  std::vector<base::FilePath> files;
  for (base::FilePath input_file = input_files.Next(); !input_file.empty();
       input_file = input_files.Next()) {
    files.push_back(input_file);
  }
  std::sort(files.begin(), files.end());

#if defined(OS_MACOSX)
  base::mac::ClearAmIBundledCache();
#endif  // defined(OS_MACOSX)

  return files;
}

// Serializes the |profiles| into a string.
std::string SerializeProfiles(const std::vector<AutofillProfile*>& profiles) {
  std::string result;
  for (size_t i = 0; i < profiles.size(); ++i) {
    result += kProfileSeparator;
    result += "\n";
    for (size_t j = 0; j < arraysize(kProfileFieldTypes); ++j) {
      ServerFieldType type = kProfileFieldTypes[j];
      base::string16 value = profiles[i]->GetRawInfo(type);
      result += AutofillType(type).ToString();
      result += kFieldSeparator;
      if (!value.empty()) {
        base::ReplaceFirstSubstringAfterOffset(
            &value, 0, base::ASCIIToUTF16("\\n"), base::ASCIIToUTF16("\n"));
        result += " ";
        result += base::UTF16ToUTF8(value);
      }
      result += "\n";
    }
  }

  return result;
}

class PersonalDataManagerMock : public PersonalDataManager {
 public:
  PersonalDataManagerMock();
  ~PersonalDataManagerMock() override;

  // Reset the saved profiles.
  void Reset();

  // PersonalDataManager:
  std::string SaveImportedProfile(const AutofillProfile& profile) override;
  const std::vector<AutofillProfile*>& web_profiles() const override;

 private:
  ScopedVector<AutofillProfile> profiles_;

  DISALLOW_COPY_AND_ASSIGN(PersonalDataManagerMock);
};

PersonalDataManagerMock::PersonalDataManagerMock()
    : PersonalDataManager("en-US") {
}

PersonalDataManagerMock::~PersonalDataManagerMock() {
}

void PersonalDataManagerMock::Reset() {
  profiles_.clear();
}

std::string PersonalDataManagerMock::SaveImportedProfile(
    const AutofillProfile& profile) {
  std::vector<AutofillProfile> profiles;
  std::string merged_guid =
      MergeProfile(profile, profiles_.get(), "en-US", &profiles);
  if (merged_guid == profile.guid())
    profiles_.push_back(new AutofillProfile(profile));
  return merged_guid;
}

const std::vector<AutofillProfile*>& PersonalDataManagerMock::web_profiles()
    const {
  return profiles_.get();
}

}  // namespace

// A data-driven test for verifying merging of Autofill profiles. Each input is
// a structured dump of a set of implicitly detected autofill profiles. The
// corresponding output file is a dump of the saved profiles that result from
// importing the input profiles. The output file format is identical to the
// input format.
class AutofillMergeTest : public DataDrivenTest,
                          public testing::TestWithParam<base::FilePath> {
 protected:
  AutofillMergeTest();
  ~AutofillMergeTest() override;

  // testing::Test:
  void SetUp() override;

  void TearDown() override;

  // DataDrivenTest:
  void GenerateResults(const std::string& input, std::string* output) override;

  // Deserializes a set of Autofill profiles from |profiles|, imports each
  // sequentially, and fills |merged_profiles| with the serialized result.
  void MergeProfiles(const std::string& profiles, std::string* merged_profiles);

  // Deserializes |str| into a field type.
  ServerFieldType StringToFieldType(const std::string& str);

  PersonalDataManagerMock personal_data_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::map<std::string, ServerFieldType> string_to_field_type_map_;

  DISALLOW_COPY_AND_ASSIGN(AutofillMergeTest);
};

AutofillMergeTest::AutofillMergeTest() : DataDrivenTest(GetTestDataDir()) {
  CountryNames::SetLocaleString("en-US");
  for (size_t i = NO_SERVER_DATA; i < MAX_VALID_FIELD_TYPE; ++i) {
    ServerFieldType field_type = static_cast<ServerFieldType>(i);
    string_to_field_type_map_[AutofillType(field_type).ToString()] = field_type;
  }
}

AutofillMergeTest::~AutofillMergeTest() {
}

void AutofillMergeTest::SetUp() {
  test::DisableSystemServices(nullptr);
  scoped_feature_list_.InitAndEnableFeature(kAutofillProfileCleanup);
}

void AutofillMergeTest::TearDown() {
  test::ReenableSystemServices();
}

void AutofillMergeTest::GenerateResults(const std::string& input,
                                        std::string* output) {
  MergeProfiles(input, output);
}

void AutofillMergeTest::MergeProfiles(const std::string& profiles,
                                      std::string* merged_profiles) {
  // Start with no saved profiles.
  personal_data_.Reset();

  // Create a test form.
  FormData form;
  form.name = base::ASCIIToUTF16("MyTestForm");
  form.origin = GURL("https://www.example.com/origin.html");
  form.action = GURL("https://www.example.com/action.html");

  // Parse the input line by line.
  std::vector<std::string> lines = base::SplitString(
      profiles, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (size_t i = 0; i < lines.size(); ++i) {
    std::string line = lines[i];
    if (line != kProfileSeparator) {
      // Add a field to the current profile.
      size_t separator_pos = line.find(kFieldSeparator);
      ASSERT_NE(std::string::npos, separator_pos)
          << "Wrong format for separator on line " << i;
      base::string16 field_type =
          base::UTF8ToUTF16(line.substr(0, separator_pos));
      do {
        ++separator_pos;
      } while (separator_pos < line.size() && line[separator_pos] == ' ');
      base::string16 value =
          base::UTF8ToUTF16(line.substr(separator_pos));
      base::ReplaceFirstSubstringAfterOffset(
          &value, 0, base::ASCIIToUTF16("\\n"), base::ASCIIToUTF16("\n"));

      FormFieldData field;
      field.label = field_type;
      field.name = field_type;
      field.value = value;
      field.form_control_type = "text";
      field.is_focusable = true;
      form.fields.push_back(field);
    }

    // The first line is always a profile separator, and the last profile is not
    // followed by an explicit separator.
    if ((i > 0 && line == kProfileSeparator) || i == lines.size() - 1) {
      // Reached the end of a profile.  Try to import it.
      FormStructure form_structure(form);
      for (size_t i = 0; i < form_structure.field_count(); ++i) {
        // Set the heuristic type for each field, which is currently serialized
        // into the field's name.
        AutofillField* field =
            const_cast<AutofillField*>(form_structure.field(i));
        ServerFieldType type =
            StringToFieldType(base::UTF16ToUTF8(field->name));
        field->set_heuristic_type(type);
      }
      form_structure.IdentifySections(false);

      // Import the profile.
      std::unique_ptr<CreditCard> imported_credit_card;
      personal_data_.ImportFormData(form_structure, false,
                                    &imported_credit_card);
      EXPECT_FALSE(imported_credit_card);

      // Clear the |form| to start a new profile.
      form.fields.clear();
    }
  }

  *merged_profiles = SerializeProfiles(personal_data_.web_profiles());
}

ServerFieldType AutofillMergeTest::StringToFieldType(const std::string& str) {
  return string_to_field_type_map_[str];
}

TEST_P(AutofillMergeTest, DataDrivenMergeProfiles) {
  RunOneDataDrivenTest(GetParam(), GetOutputDirectory(kTestName));
}

INSTANTIATE_TEST_CASE_P(, AutofillMergeTest, testing::ValuesIn(GetTestFiles()));

}  // namespace autofill

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/flags_ui/flags_state.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/format_macros.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/flags_ui/feature_entry.h"
#include "components/flags_ui/flags_ui_pref_names.h"
#include "components/flags_ui/flags_ui_switches.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/variations_associated_data.h"
#include "grit/components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace flags_ui {

namespace {

const char kFlags1[] = "flag1";
const char kFlags2[] = "flag2";
const char kFlags3[] = "flag3";
const char kFlags4[] = "flag4";
const char kFlags5[] = "flag5";
const char kFlags6[] = "flag6";
const char kFlags7[] = "flag7";
const char kFlags8[] = "flag8";

const char kSwitch1[] = "switch";
const char kSwitch2[] = "switch2";
const char kSwitch3[] = "switch3";
const char kSwitch6[] = "switch6";
const char kValueForSwitch2[] = "value_for_switch2";

const char kMultiSwitch1[] = "multi_switch1";
const char kMultiSwitch2[] = "multi_switch2";
const char kValueForMultiSwitch2[] = "value_for_multi_switch2";

const char kEnableDisableValue1[] = "value1";
const char kEnableDisableValue2[] = "value2";

const char kEnableFeatures[] = "dummy-enable-features";
const char kDisableFeatures[] = "dummy-disable-features";

const char kTestTrial[] = "TestTrial";
const char kTestParam[] = "param";
const char kTestParamValue[] = "value";

const base::Feature kTestFeature1{"FeatureName1",
                                  base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kTestFeature2{"FeatureName2",
                                  base::FEATURE_ENABLED_BY_DEFAULT};

const FeatureEntry::FeatureParam kTestVariationOther[] = {
    {kTestParam, kTestParamValue}};

const FeatureEntry::FeatureVariation kTestVariations[] = {
    {"dummy description", kTestVariationOther, 1, nullptr}};

// Those have to be valid ids for the translation system but the value are
// never used, so pick one at random from the current component.
const int kDummyNameId = IDS_FLAGS_UI_WARNING_HEADER;
const int kDummyDescriptionId = IDS_FLAGS_UI_WARNING_TEXT;

bool SkipFeatureEntry(const FeatureEntry& feature_entry) {
  return false;
}

}  // namespace

const FeatureEntry::Choice kMultiChoices[] = {
    {kDummyDescriptionId, "", ""},
    {kDummyDescriptionId, kMultiSwitch1, ""},
    {kDummyDescriptionId, kMultiSwitch2, kValueForMultiSwitch2},
};

// The entries that are set for these tests. The 3rd entry is not supported on
// the current platform, all others are.
static FeatureEntry kEntries[] = {
    {kFlags1, kDummyNameId, kDummyDescriptionId,
     0,  // Ends up being mapped to the current platform.
     FeatureEntry::SINGLE_VALUE, kSwitch1, "", nullptr, nullptr, nullptr, 0,
     nullptr, nullptr, nullptr},
    {kFlags2, kDummyNameId, kDummyDescriptionId,
     0,  // Ends up being mapped to the current platform.
     FeatureEntry::SINGLE_VALUE, kSwitch2, kValueForSwitch2, nullptr, nullptr,
     nullptr, 0, nullptr, nullptr, nullptr},
    {kFlags3, kDummyNameId, kDummyDescriptionId,
     0,  // This ends up enabling for an OS other than the current.
     FeatureEntry::SINGLE_VALUE, kSwitch3, "", nullptr, nullptr, nullptr, 0,
     nullptr, nullptr, nullptr},
    {kFlags4, kDummyNameId, kDummyDescriptionId,
     0,  // Ends up being mapped to the current platform.
     FeatureEntry::MULTI_VALUE, "", "", "", "", nullptr,
     arraysize(kMultiChoices), kMultiChoices, nullptr, nullptr},
    {kFlags5, kDummyNameId, kDummyDescriptionId,
     0,  // Ends up being mapped to the current platform.
     FeatureEntry::ENABLE_DISABLE_VALUE, kSwitch1, kEnableDisableValue1,
     kSwitch2, kEnableDisableValue2, nullptr, 3, nullptr, nullptr, nullptr},
    {kFlags6, kDummyNameId, kDummyDescriptionId, 0,
     FeatureEntry::SINGLE_DISABLE_VALUE, kSwitch6, "", nullptr, nullptr,
     nullptr, 0, nullptr, nullptr, nullptr},
    {kFlags7, kDummyNameId, kDummyDescriptionId,
     0,  // Ends up being mapped to the current platform.
     FeatureEntry::FEATURE_VALUE, nullptr, nullptr, nullptr, nullptr,
     &kTestFeature1, 3, nullptr, nullptr, nullptr},
    {kFlags8, kDummyNameId, kDummyDescriptionId,
     0,  // Ends up being mapped to the current platform.
     FeatureEntry::FEATURE_WITH_VARIATIONS_VALUE, nullptr, nullptr, nullptr,
     nullptr, &kTestFeature2, 4, nullptr, kTestVariations, kTestTrial},
};

class FlagsStateTest : public ::testing::Test {
 protected:
  FlagsStateTest() : flags_storage_(&prefs_), trial_list_(nullptr) {
    prefs_.registry()->RegisterListPref(prefs::kEnabledLabsExperiments);

    for (size_t i = 0; i < arraysize(kEntries); ++i)
      kEntries[i].supported_platforms = FlagsState::GetCurrentPlatform();

    int os_other_than_current = 1;
    while (os_other_than_current == FlagsState::GetCurrentPlatform())
      os_other_than_current <<= 1;
    kEntries[2].supported_platforms = os_other_than_current;
    flags_state_.reset(new FlagsState(kEntries, arraysize(kEntries)));
  }

  ~FlagsStateTest() override {
    variations::testing::ClearAllVariationParams();
  }

  TestingPrefServiceSimple prefs_;
  PrefServiceFlagsStorage flags_storage_;
  std::unique_ptr<FlagsState> flags_state_;
  base::FieldTrialList trial_list_;
};

TEST_F(FlagsStateTest, NoChangeNoRestart) {
  EXPECT_FALSE(flags_state_->IsRestartNeededToCommitChanges());
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, false);
  EXPECT_FALSE(flags_state_->IsRestartNeededToCommitChanges());

  // kFlags6 is enabled by default, so enabling should not require a restart.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags6, true);
  EXPECT_FALSE(flags_state_->IsRestartNeededToCommitChanges());
}

TEST_F(FlagsStateTest, ChangeNeedsRestart) {
  EXPECT_FALSE(flags_state_->IsRestartNeededToCommitChanges());
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);
  EXPECT_TRUE(flags_state_->IsRestartNeededToCommitChanges());
}

// Tests that disabling a default enabled entry requires a restart.
TEST_F(FlagsStateTest, DisableChangeNeedsRestart) {
  EXPECT_FALSE(flags_state_->IsRestartNeededToCommitChanges());
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags6, false);
  EXPECT_TRUE(flags_state_->IsRestartNeededToCommitChanges());
}

TEST_F(FlagsStateTest, MultiFlagChangeNeedsRestart) {
  const FeatureEntry& entry = kEntries[3];
  ASSERT_EQ(kFlags4, entry.internal_name);
  EXPECT_FALSE(flags_state_->IsRestartNeededToCommitChanges());
  // Enable the 2nd choice of the multi-value.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(2),
                                       true);
  EXPECT_TRUE(flags_state_->IsRestartNeededToCommitChanges());
  flags_state_->Reset();
  EXPECT_FALSE(flags_state_->IsRestartNeededToCommitChanges());
  // Enable the default choice now.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(0),
                                       true);
  EXPECT_TRUE(flags_state_->IsRestartNeededToCommitChanges());
}

TEST_F(FlagsStateTest, AddTwoFlagsRemoveOne) {
  // Add two entries, check they're there.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags2, true);

  const base::ListValue* entries_list =
      prefs_.GetList(prefs::kEnabledLabsExperiments);
  ASSERT_TRUE(entries_list != nullptr);

  ASSERT_EQ(2u, entries_list->GetSize());

  std::string s0;
  ASSERT_TRUE(entries_list->GetString(0, &s0));
  std::string s1;
  ASSERT_TRUE(entries_list->GetString(1, &s1));

  EXPECT_TRUE(s0 == kFlags1 || s1 == kFlags1);
  EXPECT_TRUE(s0 == kFlags2 || s1 == kFlags2);

  // Remove one entry, check the other's still around.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags2, false);

  entries_list = prefs_.GetList(prefs::kEnabledLabsExperiments);
  ASSERT_TRUE(entries_list != nullptr);
  ASSERT_EQ(1u, entries_list->GetSize());
  ASSERT_TRUE(entries_list->GetString(0, &s0));
  EXPECT_TRUE(s0 == kFlags1);
}

TEST_F(FlagsStateTest, AddTwoFlagsRemoveBoth) {
  // Add two entries, check the pref exists.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags2, true);
  const base::ListValue* entries_list =
      prefs_.GetList(prefs::kEnabledLabsExperiments);
  ASSERT_TRUE(entries_list != nullptr);

  // Remove both, the pref should have been removed completely.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, false);
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags2, false);
  entries_list = prefs_.GetList(prefs::kEnabledLabsExperiments);
  EXPECT_TRUE(entries_list == nullptr || entries_list->GetSize() == 0);
}

TEST_F(FlagsStateTest, ConvertFlagsToSwitches) {
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);

  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitch("foo");

  EXPECT_TRUE(command_line.HasSwitch("foo"));
  EXPECT_FALSE(command_line.HasSwitch(kSwitch1));

  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                       kAddSentinels, kEnableFeatures,
                                       kDisableFeatures);

  EXPECT_TRUE(command_line.HasSwitch("foo"));
  EXPECT_TRUE(command_line.HasSwitch(kSwitch1));
  EXPECT_TRUE(command_line.HasSwitch(switches::kFlagSwitchesBegin));
  EXPECT_TRUE(command_line.HasSwitch(switches::kFlagSwitchesEnd));

  base::CommandLine command_line2(base::CommandLine::NO_PROGRAM);

  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line2,
                                       kNoSentinels, kEnableFeatures,
                                       kDisableFeatures);

  EXPECT_TRUE(command_line2.HasSwitch(kSwitch1));
  EXPECT_FALSE(command_line2.HasSwitch(switches::kFlagSwitchesBegin));
  EXPECT_FALSE(command_line2.HasSwitch(switches::kFlagSwitchesEnd));
}

TEST_F(FlagsStateTest, RegisterAllFeatureVariationParameters) {
  const FeatureEntry& entry = kEntries[7];
  std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);

  // Select the "Default" variation.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(0),
                                       true);
  flags_state_->RegisterAllFeatureVariationParameters(&flags_storage_,
                                                      feature_list.get());
  // No value should be associated.
  EXPECT_EQ("", variations::GetVariationParamValue(kTestTrial, kTestParam));
  // The trial should not be created.
  base::FieldTrial* trial = base::FieldTrialList::Find(kTestTrial);
  EXPECT_EQ(nullptr, trial);

  // Select the default "Enabled" variation.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(1),
                                       true);

  flags_state_->RegisterAllFeatureVariationParameters(&flags_storage_,
                                                      feature_list.get());
  // No value should be associated as this is the default option.
  EXPECT_EQ("",
            variations::GetVariationParamValue(kTestTrial, kTestParam));

  // The trial should be created.
  trial = base::FieldTrialList::Find(kTestTrial);
  EXPECT_NE(nullptr, trial);
  // The about:flags group should be selected for the trial.
  EXPECT_EQ(internal::kTrialGroupAboutFlags, trial->group_name());

  // Select the only one variation.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(2),
                                       true);
  flags_state_->RegisterAllFeatureVariationParameters(&flags_storage_,
                                                      feature_list.get());
  // Associating for the second time should not change the value.
  EXPECT_EQ("",
            variations::GetVariationParamValue(kTestTrial, kTestParam));
}

TEST_F(FlagsStateTest, RegisterAllFeatureVariationParametersNonDefault) {
  const FeatureEntry& entry = kEntries[7];
  std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);

  // Select the only one variation.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(2),
                                       true);
  flags_state_->RegisterAllFeatureVariationParameters(&flags_storage_,
                                                      feature_list.get());

  // Set the feature_list as the main instance so that
  // variations::GetVariationParamValueByFeature below works.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatureList(std::move(feature_list));

  // The param should have the value predefined in this variation.
  EXPECT_EQ(kTestParamValue,
            variations::GetVariationParamValue(kTestTrial, kTestParam));

  // The value should be associated also via the name of the feature.
  EXPECT_EQ(kTestParamValue, variations::GetVariationParamValueByFeature(
                                  kTestFeature2, kTestParam));
}

base::CommandLine::StringType CreateSwitch(const std::string& value) {
#if defined(OS_WIN)
  return base::ASCIIToUTF16(value);
#else
  return value;
#endif
}

TEST_F(FlagsStateTest, CompareSwitchesToCurrentCommandLine) {
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);

  const std::string kDoubleDash("--");

  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitch("foo");

  base::CommandLine new_command_line(base::CommandLine::NO_PROGRAM);
  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &new_command_line,
                                       kAddSentinels, kEnableFeatures,
                                       kDisableFeatures);

  EXPECT_FALSE(FlagsState::AreSwitchesIdenticalToCurrentCommandLine(
      new_command_line, command_line, nullptr, nullptr, nullptr));
  {
    std::set<base::CommandLine::StringType> difference;
    EXPECT_FALSE(FlagsState::AreSwitchesIdenticalToCurrentCommandLine(
        new_command_line, command_line, &difference, nullptr, nullptr));
    EXPECT_EQ(1U, difference.size());
    EXPECT_EQ(1U, difference.count(CreateSwitch(kDoubleDash + kSwitch1)));
  }

  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                       kAddSentinels, kEnableFeatures,
                                       kDisableFeatures);

  EXPECT_TRUE(FlagsState::AreSwitchesIdenticalToCurrentCommandLine(
      new_command_line, command_line, nullptr, nullptr, nullptr));
  {
    std::set<base::CommandLine::StringType> difference;
    EXPECT_TRUE(FlagsState::AreSwitchesIdenticalToCurrentCommandLine(
        new_command_line, command_line, &difference, nullptr, nullptr));
    EXPECT_TRUE(difference.empty());
  }

  // Now both have flags but different.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, false);
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags2, true);

  base::CommandLine another_command_line(base::CommandLine::NO_PROGRAM);
  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &another_command_line,
                                       kAddSentinels, kEnableFeatures,
                                       kDisableFeatures);

  EXPECT_FALSE(FlagsState::AreSwitchesIdenticalToCurrentCommandLine(
      new_command_line, another_command_line, nullptr, nullptr, nullptr));
  {
    std::set<base::CommandLine::StringType> difference;
    EXPECT_FALSE(FlagsState::AreSwitchesIdenticalToCurrentCommandLine(
        new_command_line, another_command_line, &difference, nullptr, nullptr));
    EXPECT_EQ(2U, difference.size());
    EXPECT_EQ(1U, difference.count(CreateSwitch(kDoubleDash + kSwitch1)));
    EXPECT_EQ(1U, difference.count(CreateSwitch(kDoubleDash + kSwitch2 + "=" +
                                                kValueForSwitch2)));
  }
}

TEST_F(FlagsStateTest, RemoveFlagSwitches) {
  std::map<std::string, base::CommandLine::StringType> switch_list;
  switch_list[kSwitch1] = base::CommandLine::StringType();
  switch_list[switches::kFlagSwitchesBegin] = base::CommandLine::StringType();
  switch_list[switches::kFlagSwitchesEnd] = base::CommandLine::StringType();
  switch_list["foo"] = base::CommandLine::StringType();

  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);

  // This shouldn't do anything before ConvertFlagsToSwitches() wasn't called.
  flags_state_->RemoveFlagsSwitches(&switch_list);
  ASSERT_EQ(4u, switch_list.size());
  EXPECT_TRUE(base::ContainsKey(switch_list, kSwitch1));
  EXPECT_TRUE(base::ContainsKey(switch_list, switches::kFlagSwitchesBegin));
  EXPECT_TRUE(base::ContainsKey(switch_list, switches::kFlagSwitchesEnd));
  EXPECT_TRUE(base::ContainsKey(switch_list, "foo"));

  // Call ConvertFlagsToSwitches(), then RemoveFlagsSwitches() again.
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitch("foo");
  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                       kAddSentinels, kEnableFeatures,
                                       kDisableFeatures);
  flags_state_->RemoveFlagsSwitches(&switch_list);

  // Now the about:flags-related switch should have been removed.
  ASSERT_EQ(1u, switch_list.size());
  EXPECT_TRUE(base::ContainsKey(switch_list, "foo"));
}

TEST_F(FlagsStateTest, RemoveFlagSwitches_Features) {
  struct {
    int enabled_choice;  // 0: default, 1: enabled, 2: disabled.
    const char* existing_enable_features;
    const char* existing_disable_features;
    const char* expected_enable_features;
    const char* expected_disable_features;
  } cases[] = {
      // Default value: Should not affect existing flags.
      {0, nullptr, nullptr, nullptr, nullptr},
      {0, "A,B", "C", "A,B", "C"},
      // "Enable" option: should only affect enabled list.
      {1, nullptr, nullptr, "FeatureName1", nullptr},
      {1, "A,B", "C", "A,B,FeatureName1", "C"},
      // "Disable" option: should only affect disabled list.
      {2, nullptr, nullptr, nullptr, "FeatureName1"},
      {2, "A,B", "C", "A,B", "C,FeatureName1"},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf(
        "Test[%" PRIuS "]: %d [%s] [%s]", i, cases[i].enabled_choice,
        cases[i].existing_enable_features ? cases[i].existing_enable_features
                                          : "null",
        cases[i].existing_disable_features ? cases[i].existing_disable_features
                                           : "null"));

    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    if (cases[i].existing_enable_features) {
      command_line.AppendSwitchASCII(kEnableFeatures,
                                     cases[i].existing_enable_features);
    }
    if (cases[i].existing_disable_features) {
      command_line.AppendSwitchASCII(kDisableFeatures,
                                     cases[i].existing_disable_features);
    }

    flags_state_->Reset();

    const std::string entry_name = base::StringPrintf(
        "%s%s%d", kFlags7, testing::kMultiSeparator, cases[i].enabled_choice);
    flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry_name, true);

    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    auto switch_list = command_line.GetSwitches();
    EXPECT_EQ(cases[i].expected_enable_features != nullptr,
              base::ContainsKey(switch_list, kEnableFeatures));
    if (cases[i].expected_enable_features)
      EXPECT_EQ(CreateSwitch(cases[i].expected_enable_features),
                switch_list[kEnableFeatures]);

    EXPECT_EQ(cases[i].expected_disable_features != nullptr,
              base::ContainsKey(switch_list, kDisableFeatures));
    if (cases[i].expected_disable_features)
      EXPECT_EQ(CreateSwitch(cases[i].expected_disable_features),
                switch_list[kDisableFeatures]);

    // RemoveFlagsSwitches() should result in the original values for these
    // switches.
    switch_list = command_line.GetSwitches();
    flags_state_->RemoveFlagsSwitches(&switch_list);
    EXPECT_EQ(cases[i].existing_enable_features != nullptr,
              base::ContainsKey(switch_list, kEnableFeatures));
    if (cases[i].existing_enable_features)
      EXPECT_EQ(CreateSwitch(cases[i].existing_enable_features),
                switch_list[kEnableFeatures]);
    EXPECT_EQ(cases[i].existing_disable_features != nullptr,
              base::ContainsKey(switch_list, kEnableFeatures));
    if (cases[i].existing_disable_features)
      EXPECT_EQ(CreateSwitch(cases[i].existing_disable_features),
                switch_list[kDisableFeatures]);
  }
}

// Tests enabling entries that aren't supported on the current platform.
TEST_F(FlagsStateTest, PersistAndPrune) {
  // Enable entries 1 and 3.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags3, true);
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  EXPECT_FALSE(command_line.HasSwitch(kSwitch1));
  EXPECT_FALSE(command_line.HasSwitch(kSwitch3));

  // Convert the flags to switches. Entry 3 shouldn't be among the switches
  // as it is not applicable to the current platform.
  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                       kAddSentinels, kEnableFeatures,
                                       kDisableFeatures);
  EXPECT_TRUE(command_line.HasSwitch(kSwitch1));
  EXPECT_FALSE(command_line.HasSwitch(kSwitch3));

  // FeatureEntry 3 should show still be persisted in preferences though.
  const base::ListValue* entries_list =
      prefs_.GetList(prefs::kEnabledLabsExperiments);
  ASSERT_TRUE(entries_list);
  EXPECT_EQ(2U, entries_list->GetSize());
  std::string s0;
  ASSERT_TRUE(entries_list->GetString(0, &s0));
  EXPECT_EQ(kFlags1, s0);
  std::string s1;
  ASSERT_TRUE(entries_list->GetString(1, &s1));
  EXPECT_EQ(kFlags3, s1);
}

// Tests that switches which should have values get them in the command
// line.
TEST_F(FlagsStateTest, CheckValues) {
  // Enable entries 1 and 2.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags1, true);
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags2, true);
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  EXPECT_FALSE(command_line.HasSwitch(kSwitch1));
  EXPECT_FALSE(command_line.HasSwitch(kSwitch2));

  // Convert the flags to switches.
  flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                       kAddSentinels, kEnableFeatures,
                                       kDisableFeatures);
  EXPECT_TRUE(command_line.HasSwitch(kSwitch1));
  EXPECT_EQ(std::string(), command_line.GetSwitchValueASCII(kSwitch1));
  EXPECT_TRUE(command_line.HasSwitch(kSwitch2));
  EXPECT_EQ(std::string(kValueForSwitch2),
            command_line.GetSwitchValueASCII(kSwitch2));

  // Confirm that there is no '=' in the command line for simple switches.
  std::string switch1_with_equals =
      std::string("--") + std::string(kSwitch1) + std::string("=");
#if defined(OS_WIN)
  EXPECT_EQ(base::string16::npos, command_line.GetCommandLineString().find(
                                      base::ASCIIToUTF16(switch1_with_equals)));
#else
  EXPECT_EQ(std::string::npos,
            command_line.GetCommandLineString().find(switch1_with_equals));
#endif

  // And confirm there is a '=' for switches with values.
  std::string switch2_with_equals =
      std::string("--") + std::string(kSwitch2) + std::string("=");
#if defined(OS_WIN)
  EXPECT_NE(base::string16::npos, command_line.GetCommandLineString().find(
                                      base::ASCIIToUTF16(switch2_with_equals)));
#else
  EXPECT_NE(std::string::npos,
            command_line.GetCommandLineString().find(switch2_with_equals));
#endif

  // And it should persist.
  const base::ListValue* entries_list =
      prefs_.GetList(prefs::kEnabledLabsExperiments);
  ASSERT_TRUE(entries_list);
  EXPECT_EQ(2U, entries_list->GetSize());
  std::string s0;
  ASSERT_TRUE(entries_list->GetString(0, &s0));
  EXPECT_EQ(kFlags1, s0);
  std::string s1;
  ASSERT_TRUE(entries_list->GetString(1, &s1));
  EXPECT_EQ(kFlags2, s1);
}

// Tests multi-value type entries.
TEST_F(FlagsStateTest, MultiValues) {
  const FeatureEntry& entry = kEntries[3];
  ASSERT_EQ(kFlags4, entry.internal_name);

  // Initially, the first "deactivated" option of the multi entry should
  // be set.
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kMultiSwitch1));
    EXPECT_FALSE(command_line.HasSwitch(kMultiSwitch2));
  }

  // Enable the 2nd choice of the multi-value.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(2),
                                       true);
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kMultiSwitch1));
    EXPECT_TRUE(command_line.HasSwitch(kMultiSwitch2));
    EXPECT_EQ(std::string(kValueForMultiSwitch2),
              command_line.GetSwitchValueASCII(kMultiSwitch2));
  }

  // Disable the multi-value entry.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(0),
                                       true);
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kMultiSwitch1));
    EXPECT_FALSE(command_line.HasSwitch(kMultiSwitch2));
  }
}

// Tests that disable flags are added when an entry is disabled.
TEST_F(FlagsStateTest, DisableFlagCommandLine) {
  // Nothing selected.
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kSwitch6));
  }

  // Disable the entry 6.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags6, false);
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_TRUE(command_line.HasSwitch(kSwitch6));
  }

  // Enable entry 6.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, kFlags6, true);
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kSwitch6));
  }
}

TEST_F(FlagsStateTest, EnableDisableValues) {
  const FeatureEntry& entry = kEntries[4];
  ASSERT_EQ(kFlags5, entry.internal_name);

  // Nothing selected.
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kSwitch1));
    EXPECT_FALSE(command_line.HasSwitch(kSwitch2));
  }

  // "Enable" option selected.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(1),
                                       true);
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_TRUE(command_line.HasSwitch(kSwitch1));
    EXPECT_FALSE(command_line.HasSwitch(kSwitch2));
    EXPECT_EQ(kEnableDisableValue1, command_line.GetSwitchValueASCII(kSwitch1));
  }

  // "Disable" option selected.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(2),
                                       true);
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kSwitch1));
    EXPECT_TRUE(command_line.HasSwitch(kSwitch2));
    EXPECT_EQ(kEnableDisableValue2, command_line.GetSwitchValueASCII(kSwitch2));
  }

  // "Default" option selected, same as nothing selected.
  flags_state_->SetFeatureEntryEnabled(&flags_storage_, entry.NameForOption(0),
                                       true);
  {
    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_FALSE(command_line.HasSwitch(kMultiSwitch1));
    EXPECT_FALSE(command_line.HasSwitch(kMultiSwitch2));
  }
}

TEST_F(FlagsStateTest, FeatureValues) {
  const FeatureEntry& entry = kEntries[6];
  ASSERT_EQ(kFlags7, entry.internal_name);

  struct {
    int enabled_choice;
    const char* existing_enable_features;
    const char* existing_disable_features;
    const char* expected_enable_features;
    const char* expected_disable_features;
  } cases[] = {
      // Nothing selected.
      {-1, nullptr, nullptr, "", ""},
      // "Default" option selected, same as nothing selected.
      {0, nullptr, nullptr, "", ""},
      // "Enable" option selected.
      {1, nullptr, nullptr, "FeatureName1", ""},
      // "Disable" option selected.
      {2, nullptr, nullptr, "", "FeatureName1"},
      // "Enable" option should get added to the existing list.
      {1, "Foo,Bar", nullptr, "Foo,Bar,FeatureName1", ""},
      // "Disable" option should get added to the existing list.
      {2, nullptr, "Foo,Bar", "", "Foo,Bar,FeatureName1"},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf(
        "Test[%" PRIuS "]: %d [%s] [%s]", i, cases[i].enabled_choice,
        cases[i].existing_enable_features ? cases[i].existing_enable_features
                                          : "null",
        cases[i].existing_disable_features ? cases[i].existing_disable_features
                                           : "null"));

    if (cases[i].enabled_choice != -1) {
      flags_state_->SetFeatureEntryEnabled(
          &flags_storage_, entry.NameForOption(cases[i].enabled_choice), true);
    }

    base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
    if (cases[i].existing_enable_features) {
      command_line.AppendSwitchASCII(kEnableFeatures,
                                     cases[i].existing_enable_features);
    }
    if (cases[i].existing_disable_features) {
      command_line.AppendSwitchASCII(kDisableFeatures,
                                     cases[i].existing_disable_features);
    }

    flags_state_->ConvertFlagsToSwitches(&flags_storage_, &command_line,
                                         kAddSentinels, kEnableFeatures,
                                         kDisableFeatures);
    EXPECT_EQ(cases[i].expected_enable_features,
              command_line.GetSwitchValueASCII(kEnableFeatures));
    EXPECT_EQ(cases[i].expected_disable_features,
              command_line.GetSwitchValueASCII(kDisableFeatures));
  }
}

TEST_F(FlagsStateTest, GetFlagFeatureEntries) {
  base::ListValue supported_entries;
  base::ListValue unsupported_entries;
  flags_state_->GetFlagFeatureEntries(&flags_storage_, kGeneralAccessFlagsOnly,
                                      &supported_entries, &unsupported_entries,
                                      base::Bind(&SkipFeatureEntry));
  // All |kEntries| except for |kFlags3| should be supported.
  EXPECT_EQ(7u, supported_entries.GetSize());
  EXPECT_EQ(1u, unsupported_entries.GetSize());
  EXPECT_EQ(arraysize(kEntries),
            supported_entries.GetSize() + unsupported_entries.GetSize());
}

}  // namespace flags_ui

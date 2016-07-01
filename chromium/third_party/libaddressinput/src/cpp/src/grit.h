// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef I18N_ADDRESSINPUT_GRIT_H_
#define I18N_ADDRESSINPUT_GRIT_H_

namespace i18n {
namespace addressinput {

// A message identifier that is guaranteed to not clash with any
// IDS_ADDRESSINPUT_I18N_* identifiers that are generated by GRIT. GRIT
// generates messages in the range from decimal 101 to 0x7FFF in order to work
// with Windows.
// https://code.google.com/p/grit-i18n/source/browse/trunk/grit/format/rc_header.py?r=94#169
// http://msdn.microsoft.com/en-us/library/t2zechd4(VS.71).aspx
//
// The enum must be named to enable using it in gtest templates, e.g.
// EXPECT_EQ(INVALID_MESSAGE_ID, my_id) will not compile on some platforms when
// the enum is unnamed.
enum MessageIdentifier { INVALID_MESSAGE_ID = 0 };

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_GRIT_H_

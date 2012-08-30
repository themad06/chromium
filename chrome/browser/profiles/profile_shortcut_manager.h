// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_SHORTCUT_MANAGER_H_
#define CHROME_BROWSER_PROFILES_PROFILE_SHORTCUT_MANAGER_H_

#include "base/file_path.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "ui/gfx/image/image.h"

class ProfileShortcutManager {
 public:
  virtual ~ProfileShortcutManager();

  static bool IsFeatureEnabled();
  static ProfileShortcutManager* Create(ProfileInfoCache* cache);

 protected:
  ProfileShortcutManager();

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileShortcutManager);
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_SHORTCUT_MANAGER_H_

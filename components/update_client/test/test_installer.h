// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_TEST_TEST_INSTALLER_H_
#define COMPONENTS_UPDATE_CLIENT_TEST_TEST_INSTALLER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "components/update_client/update_client.h"

namespace base {
class DictionaryValue;
}

namespace update_client {

// A TestInstaller is an installer that does nothing for installation except
// increment a counter.
class TestInstaller : public ComponentInstaller {
 public:
  TestInstaller();

  void OnUpdateError(int error) override;

  bool Install(const base::DictionaryValue& manifest,
               const base::FilePath& unpack_path) override;

  bool GetInstalledFile(const std::string& file,
                        base::FilePath* installed_file) override;

  int error() const;

  int install_count() const;

 protected:
  int error_;
  int install_count_;
};

// A ReadOnlyTestInstaller is an installer that knows about files in an existing
// directory. It will not write to the directory.
class ReadOnlyTestInstaller : public TestInstaller {
 public:
  explicit ReadOnlyTestInstaller(const base::FilePath& installed_path);

  ~ReadOnlyTestInstaller() override;

  bool GetInstalledFile(const std::string& file,
                        base::FilePath* installed_file) override;

 private:
  base::FilePath install_directory_;
};

// A VersionedTestInstaller is an installer that installs files into versioned
// directories (e.g. somedir/25.23.89.141/<files>).
class VersionedTestInstaller : public TestInstaller {
 public:
  VersionedTestInstaller();

  ~VersionedTestInstaller() override;

  bool Install(const base::DictionaryValue& manifest,
               const base::FilePath& unpack_path) override;

  bool GetInstalledFile(const std::string& file,
                        base::FilePath* installed_file) override;

 private:
  base::FilePath install_directory_;
  Version current_version_;
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_TEST_TEST_INSTALLER_H_

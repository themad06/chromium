// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_PLATFORM_KEYS_PLATFORM_KEYS_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_PLATFORM_KEYS_PLATFORM_KEYS_API_H_

#include <string>

#include "chrome/browser/extensions/chrome_extension_function.h"

namespace extensions {
namespace platform_keys {

extern const char kErrorInvalidToken[];

// Returns whether |token_id| references a known Token.
bool ValidateToken(const std::string& token_id,
                   std::string* platform_keys_token_id);

// Converts a token id from ::chromeos::platform_keys to the platformKeys API
// token id.
std::string PlatformKeysTokenIdToApiId(
    const std::string& platform_keys_token_id);

}  // namespace platform_keys

class PlatformKeysInternalSignFunction
    : public ChromeUIThreadExtensionFunction {
 private:
  ~PlatformKeysInternalSignFunction() override;
  ResponseAction Run() override;

  // Called when the signature was generated. If an error occurred,
  // |signature| will be empty and instead |error_message| be set.
  void OnSigned(const std::string& signature, const std::string& error_message);

  DECLARE_EXTENSION_FUNCTION("platformKeysInternal.sign",
                             PLATFORMKEYSINTERNAL_SIGN);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_PLATFORM_KEYS_PLATFORM_KEYS_API_H_

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_INCIDENT_REPORTING_OMNIBOX_INCIDENT_HANDLERS_H_
#define CHROME_BROWSER_SAFE_BROWSING_INCIDENT_REPORTING_OMNIBOX_INCIDENT_HANDLERS_H_

#include <stdint.h>

#include <string>

namespace safe_browsing {

class ClientIncidentReport_IncidentData;

// Returns the key of the suspicious omnibox interaction.
std::string GetOmniboxIncidentKey(
    const ClientIncidentReport_IncidentData& incident_data);

// Returns a digest of the suspicious omnibox interaction.
uint32_t GetOmniboxIncidentDigest(
    const ClientIncidentReport_IncidentData& incident_data);

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_INCIDENT_REPORTING_OMNIBOX_INCIDENT_HANDLERS_H_

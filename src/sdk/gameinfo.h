#pragma once

namespace gameinfo {

// Reads ClientVersion from steam.inf. NOTE: this is the Steam content version
// (a seven-digit number), NOT the engine build number cs2-dumper reports. It is
// informational only — never compare it with offsets::kDumpBuild.
int ClientVersion();

// The engine build number, read from engine2.dll's dwBuildNumber global. This
// is what cs2-dumper's build_number refers to, so it is the one worth
// comparing. Returns 0 when the signature does not match.
int BuildNumber();

// Logs the game build next to offsets::kDumpBuild and shouts if they differ.
// Returns true when they match.
bool ReportBuildMatch();

} // namespace gameinfo

#pragma once

namespace settings {

// Reads %TEMP%\4fir\loader_settings.ini - the file the loader writes right
// before it injects - and applies what it finds to cfg::c. Returns false when
// the file is absent, which is the normal case for a manual inject rather than
// an error: the baked defaults simply stay in place.
bool Load();

} // namespace settings

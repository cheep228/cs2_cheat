#pragma once

namespace resolve {

struct Result {
    int globalsFound = 0;
    int globalsTotal = 0;
    int fieldsFound  = 0;
    int fieldsTotal  = 0;
    int schemaClasses = 0;
    bool schemaOk = false;
    bool anyDisagreement = false;   // a resolved value differed from the baked one
};

// Reads the real offsets out of the running game and overwrites the baked
// defaults in offsets.h. Safe to call once client.dll is loaded.
Result Run();

const Result& Last();

// Writes everything resolved, plus the full field list of the classes the ESP
// touches, to <temp>\cs2_overlay_offsets.txt for sending back.
bool DumpToFile();

} // namespace resolve

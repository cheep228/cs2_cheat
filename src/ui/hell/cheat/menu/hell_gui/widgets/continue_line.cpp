#include "continue_line.h"

using namespace ImGui;

void hell::continue_line( float offset_from_start_x, float spacing ) {
	ImGui::SameLine( offset_from_start_x, spacing );
}

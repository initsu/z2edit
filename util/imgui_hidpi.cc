namespace {
float hidpi_scale = 1.0f;
}  // namespace

void ImGui_ImplSdl_SetHiDPIScale(float scale) {
    hidpi_scale = scale;
}

float ImGui_ImplSdl_GetHiDPIScale() {
    return hidpi_scale;
}
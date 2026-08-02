// ===== محاكاة واجهة تسجيل الدخول =====
static char licenseKey[64] = "";
static bool isLoggingIn = false;
static bool showError = false;
static std::string errorMsg = "";

// إعدادات النافذة
ImGui::SetNextWindowSize(ImVec2(450, 620), ImGuiCond_Always);
ImGui::SetNextWindowPos(ImVec2(300, 80), ImGuiCond_Always);

ImGui::Begin("##LoginCard", nullptr, 
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | 
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

ImDrawList* dl = ImGui::GetWindowDrawList();
ImVec2 winPos = ImGui::GetWindowPos();
ImVec2 winSize = ImGui::GetWindowSize();

// خلفية داكنة
dl->AddRectFilled(winPos, winPos + winSize, IM_COL32(20, 20, 25, 255), 30.0f);

// إطار أزرق خارجي
dl->AddRect(winPos, winPos + winSize, IM_COL32(0, 127, 255, 200), 30.0f, 0, 8.0f);
dl->AddRect(winPos + ImVec2(2, 2), winPos + winSize - ImVec2(2, 2), 
    IM_COL32(0, 127, 255, 40), 30.0f, 0, 1.5f);

// عنوان WATAN - Spidermod
ImGui::SetCursorPosY(40);
ImGui::SetWindowFontScale(1.8f);
ImVec2 titleSize = ImGui::CalcTextSize("WATAN - Spidermod");
ImGui::SetCursorPosX((winSize.x - titleSize.x) * 0.5f);
ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "WATAN - Spidermod");
ImGui::SetWindowFontScale(1.0f);

// رسالة خطأ
if (showError) {
    ImGui::SetCursorPosY(140);
    ImGui::SetWindowFontScale(0.85f);
    ImVec2 errSize = ImGui::CalcTextSize(errorMsg.c_str());
    ImGui::SetCursorPosX((winSize.x - errSize.x) * 0.5f);
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", errorMsg.c_str());
    ImGui::SetWindowFontScale(1.0f);
}

// صندوق إدخال المفتاح
ImGui::SetCursorPosY(180);
ImGui::SetCursorPosX(35);
ImVec2 inputSize = ImVec2(winSize.x - 70, 80);
ImVec2 inputPos = ImGui::GetCursorScreenPos();

dl->AddRectFilled(inputPos, inputPos + inputSize, IM_COL32(35, 35, 40, 255), 20.0f);
dl->AddRect(inputPos, inputPos + inputSize, IM_COL32(0, 127, 255, 40), 20.0f, 0, 2.0f);

// نص المفتاح
ImGui::SetCursorPosY(205);
ImGui::SetCursorPosX(55);
ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
ImGui::PushItemWidth(inputSize.x - 80);

if (strlen(licenseKey) == 0) {
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.5f, 1.0f), "XXXXX-XXXXX-XXXXX-XXXXX-XXXXX");
    ImGui::SetWindowFontScale(1.0f);
} else {
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("%s", licenseKey);
    ImGui::SetWindowFontScale(1.0f);
}

ImGui::PopItemWidth();
ImGui::PopStyleVar();
ImGui::PopStyleColor();

// أيقونة اللصق
ImVec2 iconPos = inputPos + ImVec2(inputSize.x - 55, 20);
dl->AddRectFilled(iconPos, iconPos + ImVec2(40, 40), IM_COL32(0, 127, 255, 50), 8.0f);
ImGui::SetCursorScreenPos(iconPos + ImVec2(12, 8));
ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "📋");

// شريط تحميل (يظهر عند المصادقة)
if (isLoggingIn) {
    ImGui::SetCursorPosY(310);
    ImGui::SetCursorPosX((winSize.x - 200) * 0.5f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Authenticating...");
    
    // شريط تقدم
    ImGui::SetCursorPosX(35);
    static float progress = 0.0f;
    progress += ImGui::GetIO().DeltaTime * 0.5f;
    if (progress > 1.0f) progress = 0.0f;
    ImGui::ProgressBar(progress, ImVec2(winSize.x - 70, 8), "");
}

// زر AUTHENTICATE
ImGui::SetCursorPosY(390);
ImGui::SetCursorPosX(35);
ImVec2 btnSize = ImVec2(winSize.x - 70, 80);

ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.45f, 1.0f, 1.0f));
ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.55f, 1.0f, 1.0f));
ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.35f, 0.8f, 1.0f));
ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);

if (ImGui::Button("🔑  AUTHENTICATE", btnSize)) {
    if (strlen(licenseKey) == 0) {
        showError = true;
        errorMsg = "Please enter a license key!";
    } else if (strcmp(licenseKey, "WATAN-TEST-KEY-2024") == 0) {
        showError = false;
        isLoggingIn = true;
        // محاكاة نجاح
    } else {
        showError = true;
        errorMsg = "Invalid License Key!";
    }
}

ImGui::PopStyleVar();
ImGui::PopStyleColor(3);

// انضم إلى المجتمع
ImGui::SetCursorPosY(510);
ImGui::SetWindowFontScale(1.0f);
ImVec2 commSize = ImGui::CalcTextSize("Join Community");
ImGui::SetCursorPosX((winSize.x - commSize.x) * 0.5f);
ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "Join Community");

// أيقونات تيليجرام
ImGui::SetCursorPosY(555);
ImGui::SetCursorPosX(70);
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.9f, 1.0f));
ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
if (ImGui::Button("📱 SpiderMod", ImVec2(140, 50))) {}
ImGui::SameLine(0, 20);
if (ImGui::Button("📱 WatanSpider", ImVec2(150, 50))) {}
ImGui::PopStyleVar();
ImGui::PopStyleColor();

ImGui::SetWindowFontScale(1.0f);
ImGui::End();

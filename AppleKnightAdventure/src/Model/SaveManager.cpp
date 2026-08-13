#include "Model/SaveManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <filesystem>
#include "raylib.h"

SaveManager& SaveManager::GetInstance() {
    static SaveManager instance;
    return instance;
}

SaveManager::SaveManager() 
    : playerName("Player")
    , coins(0)
    , isFirstTimePlaying(true)
{
    unlockedCharacters.push_back("Knight");
}

bool SaveManager::Load(const std::string& path) {
    char* data = LoadFileText(path.c_str());
    const std::string backupPath = path + ".bak";
    if (!data) data = LoadFileText(backupPath.c_str());
    if (!data) {
        // Use defaults
        playerName = "Player";
        coins = 0;
        isFirstTimePlaying = true;
        unlockedCharacters = {"Knight"};
        levelHighScores.clear();
        levelBestStars.clear();
        levelBestTimesMs.clear();
        return false;
    }

    std::string jsonStr(data);
    UnloadFileText(data);

    // Simple manual parsing. Very basic and assumes a certain structure.
    auto extractString = [&](const std::string& key) -> std::string {
        size_t pos = jsonStr.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = jsonStr.find("\"", pos + key.length() + 2);
        if (pos == std::string::npos) return "";
        size_t endPos = jsonStr.find("\"", pos + 1);
        if (endPos == std::string::npos) return "";
        return jsonStr.substr(pos + 1, endPos - pos - 1);
    };

    auto extractInt = [&](const std::string& key, int defaultVal) -> int {
        size_t pos = jsonStr.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = jsonStr.find(":", pos + key.length() + 2);
        if (pos == std::string::npos) return defaultVal;
        pos++;
        while (pos < jsonStr.length() && (jsonStr[pos] == ' ' || jsonStr[pos] == '\t')) pos++;
        size_t endPos = pos;
        while (endPos < jsonStr.length() && (isdigit(jsonStr[endPos]) || jsonStr[endPos] == '-')) endPos++;
        if (endPos == pos) return defaultVal;
        return std::stoi(jsonStr.substr(pos, endPos - pos));
    };

    auto extractBool = [&](const std::string& key, bool defaultVal) -> bool {
        size_t pos = jsonStr.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = jsonStr.find(":", pos + key.length() + 2);
        if (pos == std::string::npos) return defaultVal;
        pos++;
        while (pos < jsonStr.length() && (jsonStr[pos] == ' ' || jsonStr[pos] == '\t')) pos++;
        if (jsonStr.substr(pos, 4) == "true") return true;
        if (jsonStr.substr(pos, 5) == "false") return false;
        return defaultVal;
    };

    auto extractStringArray = [&](const std::string& key) -> std::vector<std::string> {
        std::vector<std::string> result;
        size_t pos = jsonStr.find("\"" + key + "\"");
        if (pos == std::string::npos) return result;
        pos = jsonStr.find("[", pos);
        if (pos == std::string::npos) return result;
        size_t endPos = jsonStr.find("]", pos);
        if (endPos == std::string::npos) return result;

        std::string arrayStr = jsonStr.substr(pos + 1, endPos - pos - 1);
        size_t strPos = 0;
        while ((strPos = arrayStr.find("\"", strPos)) != std::string::npos) {
            size_t strEndPos = arrayStr.find("\"", strPos + 1);
            if (strEndPos != std::string::npos) {
                result.push_back(arrayStr.substr(strPos + 1, strEndPos - strPos - 1));
                strPos = strEndPos + 1;
            } else {
                break;
            }
        }
        return result;
    };

    auto extractMap = [&](const std::string& key) -> std::map<int, int> {
        std::map<int, int> result;
        size_t pos = jsonStr.find("\"" + key + "\"");
        if (pos == std::string::npos) return result;
        pos = jsonStr.find("{", pos);
        if (pos == std::string::npos) return result;
        size_t endPos = jsonStr.find("}", pos);
        if (endPos == std::string::npos) return result;
        
        std::string objStr = jsonStr.substr(pos + 1, endPos - pos - 1);
        size_t curPos = 0;
        while ((curPos = objStr.find("\"", curPos)) != std::string::npos) {
            size_t keyEnd = objStr.find("\"", curPos + 1);
            if (keyEnd == std::string::npos) break;
            std::string kStr = objStr.substr(curPos + 1, keyEnd - curPos - 1);
            int k = std::stoi(kStr);
            
            size_t colonPos = objStr.find(":", keyEnd + 1);
            if (colonPos == std::string::npos) break;
            colonPos++;
            while (colonPos < objStr.length() && (objStr[colonPos] == ' ' || objStr[colonPos] == '\t')) colonPos++;
            size_t valEnd = colonPos;
            while (valEnd < objStr.length() && (isdigit(objStr[valEnd]) || objStr[valEnd] == '-')) valEnd++;
            if (valEnd > colonPos) {
                int v = std::stoi(objStr.substr(colonPos, valEnd - colonPos));
                result[k] = v;
            }
            curPos = valEnd;
        }
        return result;
    };

    std::string name = extractString("playerName");
    if (!name.empty()) playerName = name;
    
    coins = extractInt("coins", 0);
    isFirstTimePlaying = extractBool("isFirstTimePlaying", true);
    
    std::vector<std::string> chars = extractStringArray("unlockedCharacters");
    if (!chars.empty()) {
        unlockedCharacters = chars;
    } else {
        unlockedCharacters = {"Knight"};
    }
    levelHighScores = extractMap("levelHighScores");
    levelBestStars = extractMap("levelBestStars");
    levelBestTimesMs = extractMap("levelBestTimesMs");
    
    std::string sChar = extractString("selectedChar");
    if (!sChar.empty()) selectedChar = sChar;
    
    std::string sPet = extractString("selectedPet");
    // allowed to be empty
    size_t petPos = jsonStr.find("\"selectedPet\"");
    if (petPos != std::string::npos) selectedPet = sPet;

    return true;
}

void SaveManager::Save(const std::string& path) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"playerName\": \"" << playerName << "\",\n";
    ss << "  \"coins\": " << coins << ",\n";
    ss << "  \"isFirstTimePlaying\": " << (isFirstTimePlaying ? "true" : "false") << ",\n";
    
    ss << "  \"unlockedCharacters\": [";
    for (size_t i = 0; i < unlockedCharacters.size(); ++i) {
        ss << "\"" << unlockedCharacters[i] << "\"";
        if (i < unlockedCharacters.size() - 1) ss << ", ";
    }
    ss << "],\n";

    ss << "  \"levelHighScores\": {";
    auto it1 = levelHighScores.begin();
    while (it1 != levelHighScores.end()) {
        ss << "\"" << it1->first << "\": " << it1->second;
        auto next = it1;
        ++next;
        if (next != levelHighScores.end()) ss << ", ";
        it1 = next;
    }
    ss << "},\n";

    ss << "  \"levelBestStars\": {";
    auto it2 = levelBestStars.begin();
    while (it2 != levelBestStars.end()) {
        ss << "\"" << it2->first << "\": " << it2->second;
        auto next = it2;
        ++next;
        if (next != levelBestStars.end()) ss << ", ";
        it2 = next;
    }
    ss << "},\n";

    ss << "  \"levelBestTimesMs\": {";
    auto it3 = levelBestTimesMs.begin();
    while (it3 != levelBestTimesMs.end()) {
        ss << "\"" << it3->first << "\": " << it3->second;
        auto next = it3;
        ++next;
        if (next != levelBestTimesMs.end()) ss << ", ";
        it3 = next;
    }
    ss << "},\n";
    ss << "  \"selectedChar\": \"" << selectedChar << "\",\n";
    ss << "  \"selectedPet\": \"" << selectedPet << "\"\n";
    ss << "}\n";

    std::string jsonStr = ss.str();
    const std::string tempPath = path + ".tmp";
    const std::string backupPath = path + ".bak";
    if (!SaveFileText(tempPath.c_str(), const_cast<char*>(jsonStr.c_str()))) {
        TraceLog(LOG_ERROR, "SAVE: Could not write temporary save file");
        return;
    }

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        std::filesystem::copy_file(path, backupPath,
            std::filesystem::copy_options::overwrite_existing, error);
        error.clear();
    }

    // The backup makes the two-step replacement recoverable on platforms
    // where rename cannot overwrite an existing file (notably Windows).
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(tempPath, path, error);
    const bool replaced = !error;
    if (!replaced) {
        // Last-resort fallback; the backup remains available for the next load.
        std::filesystem::copy_file(tempPath, path,
            std::filesystem::copy_options::overwrite_existing, error);
        std::filesystem::remove(tempPath, error);
    }
}

std::string SaveManager::GetPlayerName() const { return playerName; }
void SaveManager::SetPlayerName(const std::string& name) { playerName = name; }

int SaveManager::GetCoins() const { return coins; }
void SaveManager::AddCoins(int amount) { coins += amount; }
void SaveManager::SpendCoins(int amount) { 
    coins -= amount;
    if (coins < 0) coins = 0;
}

bool SaveManager::IsCharUnlocked(const std::string& charName) const {
    return std::find(unlockedCharacters.begin(), unlockedCharacters.end(), charName) != unlockedCharacters.end();
}

void SaveManager::UnlockChar(const std::string& charName) {
    if (!IsCharUnlocked(charName)) {
        unlockedCharacters.push_back(charName);
    }
}

bool SaveManager::IsFirstTimePlaying() const { return isFirstTimePlaying; }
void SaveManager::SetFirstTimePlaying(bool v) { isFirstTimePlaying = v; }

int SaveManager::GetLevelHighScore(int level) const {
    auto it = levelHighScores.find(level);
    if (it != levelHighScores.end()) return it->second;
    return 0;
}

void SaveManager::SetLevelHighScore(int level, int score) {
    levelHighScores[level] = std::max(GetLevelHighScore(level), score);
}

int SaveManager::GetLevelBestStars(int level) const {
    auto it = levelBestStars.find(level);
    if (it != levelBestStars.end()) return it->second;
    return 0;
}

void SaveManager::SetLevelBestStars(int level, int stars) {
    levelBestStars[level] = std::max(GetLevelBestStars(level), stars);
}

float SaveManager::GetLevelBestTime(int level) const {
    auto it = levelBestTimesMs.find(level);
    if (it == levelBestTimesMs.end() || it->second <= 0) return 0.0f;
    return it->second / 1000.0f;
}

void SaveManager::SetLevelBestTime(int level, float seconds) {
    if (seconds <= 0.0f) return;
    const int milliseconds = std::max(1, static_cast<int>(seconds * 1000.0f + 0.5f));
    auto it = levelBestTimesMs.find(level);
    if (it == levelBestTimesMs.end() || milliseconds < it->second) {
        levelBestTimesMs[level] = milliseconds;
    }
}

std::string SaveManager::GetSelectedChar() const { return selectedChar; }
void SaveManager::SetSelectedChar(const std::string& charId) { selectedChar = charId; }

std::string SaveManager::GetSelectedPet() const { return selectedPet; }
void SaveManager::SetSelectedPet(const std::string& petId) { selectedPet = petId; }

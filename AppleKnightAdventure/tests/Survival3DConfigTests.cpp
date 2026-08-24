#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

nlohmann::json loadJson(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "cannot open " + path.string());
    nlohmann::json value;
    input >> value;
    return value;
}

std::uint32_t readLittleEndianU32(const std::vector<unsigned char>& bytes,
                                  std::size_t offset,
                                  const std::filesystem::path& path) {
    require(offset + 4 <= bytes.size(), "truncated GLB integer in " + path.string());
    return static_cast<std::uint32_t>(bytes[offset])
         | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u)
         | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u)
         | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}

nlohmann::json loadGlbJsonChunk(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    require(static_cast<bool>(input), "cannot open GLB " + path.string());
    const std::streamoff end = input.tellg();
    require(end >= 20, "GLB is too small " + path.string());
    input.seekg(0, std::ios::beg);
    std::vector<unsigned char> bytes(static_cast<std::size_t>(end));
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    require(static_cast<bool>(input), "cannot read GLB " + path.string());

    constexpr std::uint32_t kGlbMagic = 0x46546c67u; // ASCII glTF, little endian.
    constexpr std::uint32_t kJsonChunk = 0x4e4f534au; // ASCII JSON, little endian.
    require(readLittleEndianU32(bytes, 0, path) == kGlbMagic,
            "invalid GLB magic " + path.string());
    require(readLittleEndianU32(bytes, 4, path) == 2u,
            "runtime GLB must use glTF 2.0 " + path.string());
    require(readLittleEndianU32(bytes, 8, path) == bytes.size(),
            "GLB declared length mismatch " + path.string());
    const std::uint32_t jsonLength = readLittleEndianU32(bytes, 12, path);
    require(jsonLength > 0 && jsonLength % 4u == 0u,
            "invalid GLB JSON chunk length " + path.string());
    require(readLittleEndianU32(bytes, 16, path) == kJsonChunk,
            "first GLB chunk must be JSON " + path.string());
    require(20u + static_cast<std::size_t>(jsonLength) <= bytes.size(),
            "truncated GLB JSON chunk " + path.string());

    std::string jsonText(reinterpret_cast<const char*>(bytes.data() + 20), jsonLength);
    while (!jsonText.empty()) {
        const unsigned char tail = static_cast<unsigned char>(jsonText.back());
        if (tail != 0 && !std::isspace(tail)) break;
        jsonText.pop_back();
    }
    require(!jsonText.empty(), "empty GLB JSON chunk " + path.string());
    try {
        return nlohmann::json::parse(jsonText);
    } catch (const nlohmann::json::exception& error) {
        throw std::runtime_error("invalid GLB JSON " + path.string() + ": " + error.what());
    }
}

const nlohmann::json& indexedJson(const nlohmann::json& values, int index,
                                  const std::string& label) {
    require(values.is_array() && index >= 0
                && static_cast<std::size_t>(index) < values.size(),
            "invalid " + label + " index " + std::to_string(index));
    return values[static_cast<std::size_t>(index)];
}

void validateWaveTable(const std::filesystem::path& root) {
    const auto data = loadJson(root / "config" / "waves.json");
    require(data.value("schemaVersion", 0) == 1, "waves schemaVersion must be 1");
    const auto& waves = data.at("waves");
    require(waves.is_array() && waves.size() == 50, "wave table must contain exactly 50 waves");

    const std::vector<std::string> bosses{
        "brood_warden", "hexeye_artillerist", "ironroot_colossus",
        "eclipse_chimera", "void_sovereign"
    };
    std::set<int> seen;
    for (const auto& entry : waves) {
        const int wave = entry.value("wave", 0);
        require(wave >= 1 && wave <= 50, "wave number outside 1..50");
        require(seen.insert(wave).second, "duplicate wave " + std::to_string(wave));
        if (wave % 10 == 0) {
            require(entry.value("boss", std::string{}) == bosses[wave / 10 - 1],
                    "incorrect boss at wave " + std::to_string(wave));
            continue;
        }
        const auto& mix = entry.at("mix");
        const double swarm = mix.value("swarm", -1.0);
        const double ranger = mix.value("ranger", -1.0);
        const double tanker = mix.value("tanker", -1.0);
        require(swarm >= 0.0 && ranger >= 0.0 && tanker >= 0.0,
                "negative enemy weight at wave " + std::to_string(wave));
        require(std::abs((swarm + ranger + tanker) - 1.0) < 0.001,
                "enemy mix must sum to 1 at wave " + std::to_string(wave));
        require(entry.value("budgetMultiplier", 1.0) >= 0.25,
                "budgetMultiplier too small at wave " + std::to_string(wave));
    }
    require(seen.size() == 50, "wave table has missing entries");
}

void validateScaling(const std::filesystem::path& root) {
    const auto data = loadJson(root / "config" / "balance.json");
    require(data.value("schemaVersion", 0) == 1, "balance schemaVersion must be 1");
    require(!data.value("balanceVersion", std::string{}).empty(), "balanceVersion is required");
    const auto& scaling = data.at("scaling");
    const auto& director = data.at("director");
    const double hpLinear = scaling.at("hpLinear").get<double>();
    const double hpQuadratic = scaling.at("hpQuadratic").get<double>();
    const double damageLinear = scaling.at("damageLinear").get<double>();
    const double damageQuadratic = scaling.at("damageQuadratic").get<double>();
    const double budgetBase = director.at("budgetBase").get<double>();
    const double budgetLinear = director.at("budgetLinear").get<double>();
    const double budgetQuadratic = director.at("budgetQuadratic").get<double>();
    const double spawnBase = director.at("spawnInterval").get<double>();
    const double spawnMin = director.at("minSpawnInterval").get<double>();
    const int activeBase = director.at("activeCapBase").get<int>();
    const double activePerWave = director.at("activeCapPerWave").get<double>();
    const int activeMax = director.at("activeCapMax").get<int>();
    require(hpLinear >= 0.0 && hpQuadratic >= 0.0, "HP scaling must be monotonic");
    require(damageLinear >= 0.0 && damageQuadratic >= 0.0, "damage scaling must be monotonic");
    require(spawnMin > 0.0 && spawnBase >= spawnMin, "invalid spawn interval range");
    require(activeBase > 0 && activeMax >= activeBase && activeMax <= 144, "invalid active enemy cap");

    double previousHp = 0.0;
    double previousDamage = 0.0;
    double previousBudget = 0.0;
    double previousSpawnInterval = std::numeric_limits<double>::max();
    int previousCap = 0;
    for (int wave = 1; wave <= 50; ++wave) {
        const double t = wave - 1.0;
        const double hp = 1.0 + hpLinear * t + hpQuadratic * t * t;
        const double damage = 1.0 + damageLinear * t + damageQuadratic * t * t;
        const double budget = budgetBase + budgetLinear * wave + budgetQuadratic * wave * wave;
        const double spawnInterval = std::max(spawnMin, spawnBase / (1.0 + 0.03 * t));
        const int activeCap = std::min(activeMax,
            activeBase + static_cast<int>(std::floor(activePerWave * wave)));
        require(hp >= previousHp && damage >= previousDamage && budget >= previousBudget,
                "scaling regressed at wave " + std::to_string(wave));
        require(spawnInterval <= previousSpawnInterval,
                "spawn interval increased at wave " + std::to_string(wave));
        require(activeCap >= previousCap && activeCap <= activeMax,
                "active cap invalid at wave " + std::to_string(wave));
        previousHp = hp;
        previousDamage = damage;
        previousBudget = budget;
        previousSpawnInterval = spawnInterval;
        previousCap = activeCap;
    }
}

void validateServiceSafety(const std::filesystem::path& root) {
    const auto data = loadJson(root / "config" / "services.json");
    require(data.value("schemaVersion", 0) == 1, "services schemaVersion must be 1");
    require(!data.value("enabled", true), "online service must remain opt-in for local builds");
    const std::string url = data.value("baseUrl", std::string{});
    require(url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0,
            "service baseUrl must use HTTP(S)");
}

void validateCharacterConcepts(const std::filesystem::path& root) {
    for (const char* name : {
             "knight_3d_turnaround_v1.png",
             "magic_caster_3d_turnaround_v1.png",
             "core_enemies_3d_lineup_v1.png",
             "bosses_wave10_30_3d_lineup_v1.png",
             "bosses_wave40_50_3d_lineup_v1.png"}) {
        const auto path = root / "concepts" / "characters" / name;
        require(std::filesystem::exists(path), "missing character concept " + path.string());
        require(std::filesystem::file_size(path) > 10000, "character concept is unexpectedly small");
    }
}

void validateGlbAnimationContract(const std::filesystem::path& path,
                                  const std::string& modelId,
                                  const nlohmann::json& contract,
                                  int sampleRate,
                                  int totalFrames) {
    const auto gltf = loadGlbJsonChunk(path);
    require(gltf.at("asset").value("version", std::string{}) == "2.0",
            "GLB asset version must be 2.0 for " + modelId);

    const int expectedJointCount = contract.value("jointCount", 0);
    const auto& expectedJointList = contract.at("joints");
    require(expectedJointCount > 0 && expectedJointList.is_array()
                && static_cast<int>(expectedJointList.size()) == expectedJointCount,
            "invalid semantic skeleton contract for " + modelId);
    std::set<std::string> expectedJoints;
    for (const auto& joint : expectedJointList) {
        require(joint.is_string() && !joint.get<std::string>().empty(),
                "empty semantic joint in contract for " + modelId);
        require(expectedJoints.insert(joint.get<std::string>()).second,
                "duplicate semantic joint in contract for " + modelId);
    }
    const std::string rootJoint = contract.value("rootJoint", std::string{});
    require(!rootJoint.empty() && expectedJoints.count(rootJoint) == 1,
            "semantic root joint is missing for " + modelId);

    const auto& nodes = gltf.at("nodes");
    const auto& accessors = gltf.at("accessors");
    const auto& skins = gltf.at("skins");
    require(skins.is_array() && skins.size() == 1,
            "runtime model must contain exactly one skin: " + modelId);
    const auto& skin = skins[0];
    const auto& jointIndices = skin.at("joints");
    require(jointIndices.is_array()
                && static_cast<int>(jointIndices.size()) == expectedJointCount,
            "skin joint count mismatch for " + modelId);
    std::set<std::string> actualJoints;
    for (const auto& jointIndexValue : jointIndices) {
        const int jointIndex = jointIndexValue.get<int>();
        const auto& jointNode = indexedJson(nodes, jointIndex, "node");
        const std::string name = jointNode.value("name", std::string{});
        require(!name.empty(), "unnamed skin joint in " + modelId);
        require(actualJoints.insert(name).second,
                "duplicate skin joint " + name + " in " + modelId);
    }
    require(actualJoints == expectedJoints,
            "semantic skin joints do not match contract for " + modelId);

    require(skin.contains("inverseBindMatrices"),
            "skin has no inverseBindMatrices accessor for " + modelId);
    const auto& inverseBind = indexedJson(
        accessors, skin.at("inverseBindMatrices").get<int>(), "inverseBindMatrices accessor");
    require(inverseBind.value("componentType", 0) == 5126
                && inverseBind.value("type", std::string{}) == "MAT4"
                && inverseBind.value("count", 0) == expectedJointCount,
            "inverse bind matrix contract mismatch for " + modelId);

    bool hasSkinnedPrimitive = false;
    const auto& meshes = gltf.at("meshes");
    require(meshes.is_array() && !meshes.empty(), "GLB has no meshes for " + modelId);
    for (const auto& mesh : meshes) {
        for (const auto& primitive : mesh.at("primitives")) {
            const auto& attributes = primitive.at("attributes");
            if (attributes.contains("JOINTS_0") && attributes.contains("WEIGHTS_0")) {
                hasSkinnedPrimitive = true;
            }
        }
    }
    require(hasSkinnedPrimitive, "GLB has no skinned primitive for " + modelId);

    const auto& animations = gltf.at("animations");
    require(animations.is_array() && animations.size() == 1,
            "runtime model must contain exactly one master animation: " + modelId);
    const auto& animation = animations[0];
    const auto& samplers = animation.at("samplers");
    const auto& channels = animation.at("channels");
    require(samplers.is_array() && !samplers.empty()
                && channels.is_array() && !channels.empty(),
            "master animation has no samples/channels for " + modelId);

    double animationStart = std::numeric_limits<double>::max();
    double animationEnd = std::numeric_limits<double>::lowest();
    bool hasFullSampledTimeline = false;
    for (const auto& sampler : samplers) {
        const auto& inputAccessor = indexedJson(
            accessors, sampler.at("input").get<int>(), "animation input accessor");
        const auto& outputAccessor = indexedJson(
            accessors, sampler.at("output").get<int>(), "animation output accessor");
        require(inputAccessor.value("componentType", 0) == 5126
                    && inputAccessor.value("type", std::string{}) == "SCALAR",
                "animation timeline must be a float scalar for " + modelId);
        const int inputCount = inputAccessor.value("count", 0);
        require(inputCount >= 2 && inputCount <= totalFrames,
                "invalid animation keyframe count for " + modelId);
        hasFullSampledTimeline = hasFullSampledTimeline || inputCount == totalFrames;
        require(inputAccessor.contains("min") && inputAccessor.at("min").is_array()
                    && inputAccessor.at("min").size() == 1
                    && inputAccessor.contains("max") && inputAccessor.at("max").is_array()
                    && inputAccessor.at("max").size() == 1,
                "animation timeline lacks duration bounds for " + modelId);
        animationStart = std::min(animationStart, inputAccessor.at("min")[0].get<double>());
        animationEnd = std::max(animationEnd, inputAccessor.at("max")[0].get<double>());

        const std::string interpolation = sampler.value("interpolation", std::string{"LINEAR"});
        const int expectedOutputCount = interpolation == "CUBICSPLINE"
            ? inputCount * 3 : inputCount;
        require(outputAccessor.value("count", 0) == expectedOutputCount,
                "animation input/output keyframe count mismatch for " + modelId);
    }
    require(hasFullSampledTimeline,
            "master animation has no 655-frame sampled timeline for " + modelId);
    const double expectedEnd = static_cast<double>(totalFrames - 1) / sampleRate;
    require(std::abs(animationStart) < 0.0001
                && std::abs(animationEnd - expectedEnd) < 0.0001,
            "animation duration must be 0.." + std::to_string(expectedEnd)
                + " seconds for " + modelId);

    std::set<std::string> animatedJoints;
    for (const auto& channel : channels) {
        const int samplerIndex = channel.at("sampler").get<int>();
        indexedJson(samplers, samplerIndex, "animation sampler");
        const auto& target = channel.at("target");
        const int targetNodeIndex = target.at("node").get<int>();
        const auto& targetNode = indexedJson(nodes, targetNodeIndex, "animation target node");
        const std::string targetPath = target.value("path", std::string{});
        require(targetPath == "translation" || targetPath == "rotation" || targetPath == "scale",
                "unsupported animation channel path for " + modelId);
        const std::string targetName = targetNode.value("name", std::string{});
        if (expectedJoints.count(targetName) == 1) animatedJoints.insert(targetName);
    }
    require(animatedJoints == expectedJoints,
            "not every semantic skin joint is animated for " + modelId);
}

void validateStaticWeaponGlb(const std::filesystem::path& path,
                             const std::string& weaponId) {
    const auto gltf = loadGlbJsonChunk(path);
    require(gltf.at("asset").value("version", std::string{}) == "2.0",
            "static weapon GLB must use glTF 2.0: " + weaponId);
    require(!gltf.contains("skins") || gltf.at("skins").empty(),
            "static weapon must not contain a skin: " + weaponId);
    require(!gltf.contains("animations") || gltf.at("animations").empty(),
            "static weapon must not contain animation: " + weaponId);

    const auto& meshes = gltf.at("meshes");
    const auto& accessors = gltf.at("accessors");
    require(meshes.is_array() && !meshes.empty(),
            "static weapon has no meshes: " + weaponId);
    require(accessors.is_array() && !accessors.empty(),
            "static weapon has no geometry accessors: " + weaponId);

    std::set<int> referencedMeshes;
    if (gltf.contains("nodes")) {
        for (const auto& node : gltf.at("nodes")) {
            require(!node.contains("skin"),
                    "static weapon node unexpectedly references a skin: " + weaponId);
            if (node.contains("mesh")) {
                const int meshIndex = node.at("mesh").get<int>();
                indexedJson(meshes, meshIndex, "static weapon mesh");
                referencedMeshes.insert(meshIndex);
            }
        }
    }
    require(!referencedMeshes.empty(),
            "static weapon mesh is not referenced by a scene node: " + weaponId);

    bool hasRenderableGeometry = false;
    for (int meshIndex : referencedMeshes) {
        const auto& mesh = indexedJson(meshes, meshIndex, "static weapon mesh");
        require(mesh.contains("primitives") && mesh.at("primitives").is_array()
                    && !mesh.at("primitives").empty(),
                "static weapon mesh has no primitives: " + weaponId);
        for (const auto& primitive : mesh.at("primitives")) {
            const auto& attributes = primitive.at("attributes");
            require(!attributes.contains("JOINTS_0") && !attributes.contains("WEIGHTS_0"),
                    "static weapon primitive contains skinning attributes: " + weaponId);
            if (!attributes.contains("POSITION")) continue;
            const auto& positions = indexedJson(
                accessors, attributes.at("POSITION").get<int>(),
                "static weapon POSITION accessor");
            require(positions.value("componentType", 0) == 5126
                        && positions.value("type", std::string{}) == "VEC3",
                    "static weapon POSITION accessor must be float VEC3: " + weaponId);
            require(positions.value("count", 0) >= 3,
                    "static weapon primitive has fewer than three vertices: " + weaponId);
            if (primitive.contains("indices")) {
                const auto& indices = indexedJson(
                    accessors, primitive.at("indices").get<int>(),
                    "static weapon index accessor");
                require(indices.value("type", std::string{}) == "SCALAR"
                            && indices.value("count", 0) >= 3,
                        "static weapon primitive has invalid indices: " + weaponId);
            }
            hasRenderableGeometry = true;
        }
    }
    require(hasRenderableGeometry,
            "static weapon has no renderable POSITION geometry: " + weaponId);
}

void validateAnimatedRoster(const std::filesystem::path& root) {
    const auto manifest = loadJson(root / "config" / "animation_manifest.json");
    require(manifest.value("schemaVersion", 0) == 1, "animation schemaVersion must be 1");
    require(manifest.value("format", std::string{}) == "glb", "runtime models must be GLB");
    const int sampleRate = manifest.value("sampleRate", 0);
    const int totalFrames = manifest.value("totalFrames", 0);
    require(sampleRate == 60, "animation sampling must be 60 FPS");
    require(manifest.value("weaponSocketBone", std::string{}) == "weapon_socket.R",
            "unexpected modular actor weapon socket joint");
    require(manifest.value("missingAssetFallback", std::string{}) == "procedural",
            "missing modular assets must use the procedural fallback");
    require(totalFrames == 655, "unexpected animation frame count");
    const auto& clips = manifest.at("clips");
    int previousEnd = -1;
    for (const char* clipName : {"idle", "run", "basic", "skillOne", "skillTwo",
                                 "ultimatePhase", "dashSpecial", "hurt", "death"}) {
        const auto& range = clips.at(clipName);
        require(range.is_array() && range.size() == 2, std::string("invalid clip ") + clipName);
        const int first = range[0].get<int>();
        const int last = range[1].get<int>();
        require(first > previousEnd && last >= first && last < 655,
                std::string("overlapping/out-of-range clip ") + clipName);
        previousEnd = last;
    }
    const auto& contracts = manifest.at("semanticSkeletonContracts");
    require(contracts.is_object() && contracts.size() == 5,
            "animation manifest must define five semantic skeleton contracts");
    const std::array<const char*, 5> expectedContractNames{
        "hero_humanoid", "humanoid", "quadruped", "floating_artillerist", "chimera"
    };
    const std::array<int, 5> expectedContractJointCounts{16, 15, 21, 10, 22};
    for (std::size_t i = 0; i < expectedContractNames.size(); ++i) {
        const std::string contractName = expectedContractNames[i];
        require(contracts.contains(contractName),
                "missing semantic skeleton contract " + contractName);
        require(contracts.at(contractName).value("jointCount", 0)
                    == expectedContractJointCounts[i],
                "wrong semantic joint count for " + contractName);
    }
    const auto& models = manifest.at("models");
    require(models.is_array() && models.size() == 10, "animated roster must have 10 models");
    const std::array<const char*, 10> expectedModelIds{
        "knight", "magic_caster", "riftling", "hex_archer", "obsidian_brute",
        "brood_warden", "hexeye_artillerist", "ironroot_colossus",
        "eclipse_chimera", "void_sovereign"
    };
    const std::array<const char*, 10> expectedModelPaths{
        "models/heroes/knight_body_animated.glb",
        "models/heroes/magic_caster_body_animated.glb",
        "models/enemies/riftling_animated.glb",
        "models/enemies/hex_archer_animated.glb",
        "models/enemies/obsidian_brute_animated.glb",
        "models/bosses/brood_warden_animated.glb",
        "models/bosses/hexeye_artillerist_animated.glb",
        "models/bosses/ironroot_colossus_animated.glb",
        "models/bosses/eclipse_chimera_animated.glb",
        "models/bosses/void_sovereign_animated.glb"
    };
    const std::array<const char*, 10> expectedContracts{
        "hero_humanoid", "hero_humanoid", "quadruped", "hero_humanoid", "humanoid",
        "quadruped", "floating_artillerist", "humanoid", "chimera", "humanoid"
    };
    std::set<std::string> seenModelIds;
    std::set<std::string> seenModelPaths;
    std::vector<std::string> glbContractFailures;
    for (std::size_t i = 0; i < models.size(); ++i) {
        const auto& model = models[i];
        require(model.is_object(), "animated model manifest entry must be an object");
        const std::string id = model.value("id", std::string{});
        const std::string relativePath = model.value("model", std::string{});
        const std::string contractName = model.value("skeletonContract", std::string{});
        require(id == expectedModelIds[i], "unexpected animated model id at roster slot "
                    + std::to_string(i));
        require(relativePath == expectedModelPaths[i], "unexpected animated model path for " + id);
        require(contractName == expectedContracts[i], "unexpected skeleton contract for " + id);
        require(seenModelIds.insert(id).second, "duplicate animated model id " + id);
        require(seenModelPaths.insert(relativePath).second,
                "duplicate animated model path " + relativePath);
        require(contracts.contains(contractName), "unknown skeleton contract for " + id);
        const auto& contract = contracts.at(contractName);
        if (contract.contains("socketJoint")) {
            require(contract.value("socketJoint", std::string{})
                        == manifest.value("weaponSocketBone", std::string{}),
                    "modular actor socket joint disagrees with global weapon socket contract");
        }
        const auto path = root / relativePath;
        require(std::filesystem::exists(path), "missing animated model " + path.string());
        require(std::filesystem::file_size(path) > 100000,
                "animated model is unexpectedly small " + path.string());
        try {
            validateGlbAnimationContract(path, id, contract, sampleRate, totalFrames);
        } catch (const std::exception& error) {
            glbContractFailures.push_back(id + ": " + error.what());
        }
    }
    if (!glbContractFailures.empty()) {
        std::string message = "GLB semantic/animation contract failures:";
        for (const auto& failure : glbContractFailures) message += "\n - " + failure;
        require(false, message);
    }
    const auto& weapons = manifest.at("weapons");
    require(weapons.is_array() && weapons.size() == 3,
            "modular weapon roster must have three models");
    const std::array<const char*, 3> expectedWeaponIds{
        "knight_greatsword", "magic_caster_staff", "hex_archer_bow"
    };
    const std::array<const char*, 3> expectedWeaponModels{
        "models/weapons/knight_greatsword.glb",
        "models/weapons/magic_caster_staff.glb",
        "models/enemies/hex_archer_bow.glb"
    };
    const std::array<bool, 3> requiredWeapons{false, false, true};
    for (std::size_t i = 0; i < weapons.size(); ++i) {
        const auto& weapon = weapons[i];
        const std::string weaponId = weapon.value("id", std::string{});
        require(weaponId == expectedWeaponIds[i],
                "unexpected modular weapon id at roster slot " + std::to_string(i));
        require(weapon.value("model", std::string{}) == expectedWeaponModels[i],
                "unexpected modular weapon model path for " + weaponId);
        require(weapon.value("required", false) == requiredWeapons[i],
                "unexpected required flag for modular weapon " + weaponId);
        const auto path = root / weapon.at("model").get<std::string>();
        if (!std::filesystem::exists(path)) {
            require(!requiredWeapons[i],
                    "missing required modular weapon " + path.string());
            continue;
        }
        // A clean procedural bow can be only a few hundred triangles and
        // therefore much smaller than the textured hero weapons.  The GLB
        // structure/geometry audit below is the meaningful completeness gate.
        require(std::filesystem::file_size(path) > 10000,
                "modular weapon is unexpectedly small " + path.string());
        validateStaticWeaponGlb(path, weaponId);
    }
    std::size_t legacySourceCount = 0;
    const auto sourceRoot = root / "source" / "blender";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceRoot)) {
        if (entry.is_regular_file() && entry.path().extension() == ".blend") {
            require(entry.file_size() > 300000,
                    "Blender high/low source is unexpectedly small " + entry.path().string());
            const std::string group = entry.path().parent_path().filename().string();
            if (group == "heroes" || group == "enemies" || group == "bosses")
                ++legacySourceCount;
        }
    }
    require(legacySourceCount == 10,
            "legacy animated roster must retain 10 editable Blender sources");

    const auto acceptance = loadJson(
        root / "production_v3" / "step5_acceptance_report.json");
    require(acceptance.value("status", std::string{}) == "complete",
            "Step 5 acceptance report is not complete");
    require(acceptance.value("runtimeLanguage", std::string{}) == "C++",
            "Survival runtime must remain C++");
    require(acceptance.value("survivalRuntimePythonFiles", -1) == 0,
            "offline Blender tooling must not leak Python into the runtime");
    const auto& acceptanceContracts = acceptance.at("contracts");
    require(acceptanceContracts.value("animatedModelCount", 0) == 10,
            "Step 5 acceptance model count drifted");
    require(acceptanceContracts.value("editableBlendCount", 0) == 10,
            "Step 5 acceptance Blender-source count drifted");
    require(acceptanceContracts.value("modularWeaponCount", 0) == 3,
            "Step 5 acceptance modular-weapon count drifted");
}

std::set<std::string> jsonStringSet(const nlohmann::json& values,
                                    const std::string& label) {
    require(values.is_array(), label + " must be an array");
    std::set<std::string> result;
    for (const auto& value : values) {
        require(value.is_string() && !value.get<std::string>().empty(),
                label + " contains an empty/non-string value");
        require(result.insert(value.get<std::string>()).second,
                label + " contains a duplicate value");
    }
    return result;
}

void validateStepSevenNamedActor(const std::filesystem::path& path,
                                 const std::string& actorId,
                                 const std::set<std::string>& expectedActions) {
    const auto gltf = loadGlbJsonChunk(path);
    require(gltf.at("asset").value("version", std::string{}) == "2.0",
            "Step 7 actor must use glTF 2.0: " + actorId);
    require(gltf.contains("meshes") && gltf.at("meshes").is_array()
                && !gltf.at("meshes").empty(),
            "Step 7 actor has no mesh: " + actorId);
    require(gltf.contains("skins") && gltf.at("skins").is_array()
                && gltf.at("skins").size() == 1,
            "Step 7 actor must contain one skin: " + actorId);
    require(gltf.contains("animations") && gltf.at("animations").is_array()
                && gltf.at("animations").size() == expectedActions.size(),
            "Step 7 actor action count mismatch: " + actorId);

    std::set<std::string> actualActions;
    for (const auto& animation : gltf.at("animations")) {
        const std::string name = animation.value("name", std::string{});
        require(!name.empty() && actualActions.insert(name).second,
                "Step 7 actor has empty/duplicate action: " + actorId);
        require(animation.contains("samplers") && animation.at("samplers").is_array()
                    && !animation.at("samplers").empty()
                    && animation.contains("channels")
                    && animation.at("channels").is_array()
                    && !animation.at("channels").empty(),
                "Step 7 action is not sampled: " + actorId + " :: " + name);
    }
    require(actualActions == expectedActions,
            "Step 7 GLB action names do not match manifest: " + actorId);
}

void validateStepSevenSkillModel(const std::filesystem::path& path,
                                 const std::string& skillId) {
    const auto gltf = loadGlbJsonChunk(path);
    require(gltf.at("asset").value("version", std::string{}) == "2.0",
            "skill model must use glTF 2.0: " + skillId);
    require(gltf.contains("materials") && gltf.at("materials").is_array()
                && !gltf.at("materials").empty(),
            "skill model has no embedded material: " + skillId);
    require(gltf.contains("skins") && gltf.at("skins").is_array()
                && gltf.at("skins").size() == 1,
            "skill model must contain its authored activation skin: " + skillId);
    require(gltf.contains("animations") && gltf.at("animations").is_array()
                && gltf.at("animations").size() == 1
                && gltf.at("animations")[0].value("name", std::string{}) == "Activate",
            "skill model must contain the Activate action: " + skillId);

    const auto& accessors = gltf.at("accessors");
    const auto& meshes = gltf.at("meshes");
    std::array<double, 3> minimum{
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max()
    };
    std::array<double, 3> maximum{
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest()
    };
    bool hasTriangles = false;
    bool hasSkinnedGeometry = false;
    bool hasMaterialBinding = false;
    for (const auto& mesh : meshes) {
        for (const auto& primitive : mesh.at("primitives")) {
            const auto& attributes = primitive.at("attributes");
            hasSkinnedGeometry = hasSkinnedGeometry
                || (attributes.contains("JOINTS_0") && attributes.contains("WEIGHTS_0"));
            hasMaterialBinding = hasMaterialBinding || primitive.contains("material");
            if (!attributes.contains("POSITION")) continue;
            const auto& positions = indexedJson(
                accessors, attributes.at("POSITION").get<int>(), "skill POSITION accessor");
            require(positions.value("type", std::string{}) == "VEC3"
                        && positions.value("count", 0) >= 3
                        && positions.contains("min") && positions.contains("max")
                        && positions.at("min").size() == 3
                        && positions.at("max").size() == 3,
                    "skill model has invalid POSITION geometry: " + skillId);
            for (std::size_t axis = 0; axis < 3; ++axis) {
                minimum[axis] = std::min(minimum[axis],
                                         positions.at("min")[axis].get<double>());
                maximum[axis] = std::max(maximum[axis],
                                         positions.at("max")[axis].get<double>());
            }
            if (primitive.contains("indices")) {
                const auto& indices = indexedJson(
                    accessors, primitive.at("indices").get<int>(), "skill index accessor");
                hasTriangles = hasTriangles || indices.value("count", 0) >= 3;
            } else {
                hasTriangles = true;
            }
        }
    }
    require(hasTriangles && hasSkinnedGeometry && hasMaterialBinding,
            "skill model is missing triangles, skin weights or material binding: " + skillId);
    for (std::size_t axis = 0; axis < 3; ++axis)
        require(maximum[axis] - minimum[axis] > 0.015,
                "skill model collapsed to a flat plane/line: " + skillId);
}

void validateStepSevenAssets(const std::filesystem::path& root) {
    const auto actors = loadJson(root / "config" / "animation_clips_step7.json");
    require(actors.value("schemaVersion", 0) == 1,
            "Step 7 animation schemaVersion must be 1");
    require(actors.value("format", std::string{}) == "glb"
                && actors.value("sampleRate", 0) == 60,
            "Step 7 named actors must be 60 FPS GLB assets");
    const auto& roster = actors.at("namedActionRuntimeCandidates");
    require(roster.is_array() && roster.size() == 10,
            "Step 7 must contain ten named-action actors");

    const std::set<std::string> heroActions{
        "idle", "walk_forward", "walk_backward", "run_forward", "run_backward",
        "strafe_left", "strafe_right", "basic_01", "basic_02", "basic_03",
        "skill_one", "skill_two", "ultimate", "dash", "hurt", "death"
    };
    const std::set<std::string> nonHeroActions{
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    };
    const std::array<const char*, 10> expectedActorIds{
        "knight", "magic_caster", "riftling", "hex_archer", "obsidian_brute",
        "brood_warden", "hexeye_artillerist", "ironroot_colossus",
        "eclipse_chimera", "void_sovereign"
    };
    std::map<std::string, std::set<std::string>> actorActions;
    for (std::size_t index = 0; index < roster.size(); ++index) {
        const auto& actor = roster[index];
        const std::string id = actor.value("id", std::string{});
        require(id == expectedActorIds[index],
                "Step 7 actor roster order/id mismatch at slot " + std::to_string(index));
        const std::set<std::string> actions = jsonStringSet(
            actor.at("namedActions"), "Step 7 actions for " + id);
        const auto& expected = index < 2 ? heroActions : nonHeroActions;
        require(actions == expected && actor.value("actionCount", 0)
                    == static_cast<int>(expected.size()),
                "Step 7 required action set is incomplete for " + id);
        const std::string relativeModel = actor.value("model", std::string{});
        require(relativeModel.rfind("models/step7/actors/", 0) == 0,
                "Step 7 actor model is outside the runtime folder: " + id);
        const auto model = root / relativeModel;
        require(std::filesystem::exists(model) && std::filesystem::file_size(model) > 100000,
                "missing Step 7 actor GLB: " + model.string());
        validateStepSevenNamedActor(model, id, expected);
        const auto blend = root / actor.at("editableBlend").get<std::string>();
        require(std::filesystem::exists(blend) && std::filesystem::file_size(blend) > 300000,
                "missing editable Step 7 actor source: " + id);
        actorActions.emplace(id, actions);
    }

    const auto skills = loadJson(root / "config" / "skill_asset_manifest.json");
    require(skills.value("schemaVersion", 0) == 1
                && skills.value("runtime", std::string{}) == "C++17/raylib",
            "invalid Step 7 skill asset runtime contract");
    require(skills.value("renderContract", std::string{}).find("never a textured plane")
                != std::string::npos,
            "Step 7 skills must prohibit flat texture-plane substitutes");
    const auto& skillList = skills.at("skills");
    require(skillList.is_array() && skillList.size() == 10,
            "Knight and Mage must each have five non-VFX skill models");
    const std::set<std::string> expectedSlots{
        "basic", "skillOne", "skillTwo", "ultimate", "dash"
    };
    std::map<std::string, std::set<std::string>> slotsByActor;
    std::set<std::string> skillIds;
    std::set<std::string> skillModels;
    for (const auto& skill : skillList) {
        const std::string id = skill.value("id", std::string{});
        const std::string actor = skill.value("actor", std::string{});
        const std::string slot = skill.value("slot", std::string{});
        require(skillIds.insert(id).second && (actor == "knight" || actor == "magic_caster"),
                "invalid/duplicate Step 7 skill identity: " + id);
        require(expectedSlots.count(slot) == 1
                    && slotsByActor[actor].insert(slot).second,
                "invalid/duplicate hero skill slot: " + actor + "." + slot);
        require(skill.value("runtimeRequired", false)
                    && !skill.value("flatTexturePlane", true),
                "hero skill is missing its required non-flat 3D asset: " + id);
        require(!skill.value("geometryType", std::string{}).empty()
                    && !skill.value("vfx", std::string{}).empty()
                    && skill.value("assetAnimationClip", std::string{}) == "Activate",
                "hero skill asset/VFX/action contract is incomplete: " + id);
        const std::string characterAction = skill.value("animationClip", std::string{});
        require(actorActions.at(actor).count(characterAction) == 1,
                "hero skill has no matching character animation: " + id);

        const std::string relativeModel = skill.value("model", std::string{});
        require(skillModels.insert(relativeModel).second,
                "two hero skills share the same required model asset: " + relativeModel);
        const auto model = root / relativeModel;
        require(std::filesystem::exists(model) && std::filesystem::file_size(model) > 10000,
                "missing required 3D skill model: " + model.string());
        validateStepSevenSkillModel(model, id);
        const auto blend = root / skill.at("editableBlend").get<std::string>();
        require(std::filesystem::exists(blend) && std::filesystem::file_size(blend) > 300000,
                "missing editable skill source: " + id);

        const auto& events = skill.at("eventTrack");
        require(events.is_array() && events.size() == 3,
                "skill model needs spawn/contact/despawn events: " + id);
        std::set<std::string> eventNames;
        double previousTime = -1.0;
        for (const auto& event : events) {
            const std::string eventName = event.value("event", std::string{});
            const double eventTime = event.value("normalizedTime", -1.0);
            require(eventNames.insert(eventName).second && eventTime >= previousTime
                        && eventTime >= 0.0 && eventTime <= 1.0,
                    "invalid skill asset event timeline: " + id);
            previousTime = eventTime;
        }
        require(eventNames == std::set<std::string>{
                    "SpawnSkillModel", "CombatContact", "DespawnSkillModel"},
                "skill asset event coverage is incomplete: " + id);
    }
    require(slotsByActor["knight"] == expectedSlots
                && slotsByActor["magic_caster"] == expectedSlots,
            "both heroes require Basic/SkillOne/SkillTwo/Ultimate/Dash model assets");

    const auto acceptance = loadJson(
        root / "production_v3" / "step7_acceptance_report.json");
    require(acceptance.value("actorCount", 0) == 10
                && acceptance.value("skillModelCount", 0) == 10
                && acceptance.value("structuralValidation", std::string{}) == "pass"
                && acceptance.value("everyHeroCombatSlotHasCharacterActionAnd3DModel", false),
            "Step 7 acceptance report does not match the production assets");
}

void validateProductionPhaseOne(const std::filesystem::path& root) {
    const auto phaseRoot = root / "production" / "phase1";
    const auto manifest = loadJson(phaseRoot / "phase1_manifest.json");
    require(manifest.value("schemaVersion", 0) == 1, "Phase 1 schemaVersion must be 1");
    require(manifest.value("status", std::string{}) == "complete", "Phase 1 is not complete");
    require(!manifest.value("rigged", true), "Phase 1 must remain unrigged");
    require(!manifest.value("animated", true), "Phase 1 must remain unanimated");
    require(std::abs(manifest.value("unitMeters", 0.0) - 1.0) < 0.0001,
            "Phase 1 must use metre units");
    require(manifest.at("sourceAxis").value("up", std::string{}) == "+Z",
            "Blender source must be Z-up");
    require(manifest.at("sourceAxis").value("forward", std::string{}) == "-Y",
            "Blender source must face -Y");

    const auto& atlases = manifest.at("materialAtlases");
    require(atlases.is_array() && atlases.size() == 5, "Phase 1 must have five material atlases");
    for (const auto& relative : atlases) {
        const auto path = root / relative.get<std::string>();
        require(std::filesystem::exists(path), "missing Phase 1 atlas " + path.string());
        require(std::filesystem::file_size(path) > 1000000,
                "Phase 1 atlas is unexpectedly small " + path.string());
    }

    const auto qaRender = root / manifest.at("qaRender").get<std::string>();
    require(std::filesystem::exists(qaRender) && std::filesystem::file_size(qaRender) > 100000,
            "missing Phase 1 visual QA render");

    const auto& actors = manifest.at("actors");
    require(actors.is_array() && actors.size() == 10, "Phase 1 must contain ten actors");
    std::set<std::string> ids;
    for (const auto& actor : actors) {
        const std::string id = actor.value("id", std::string{});
        require(!id.empty() && ids.insert(id).second, "duplicate or empty Phase 1 actor id");
        require(actor.value("lowVertices", 0) >= 300, "Phase 1 low mesh is too sparse for " + id);
        require(actor.value("highVertices", 0) > actor.value("lowVertices", 0),
                "Phase 1 high mesh must exceed low mesh for " + id);
        require(actor.value("materials", 0) >= 1, "Phase 1 actor has no material: " + id);
        require(actor.value("uvMaps", 0) == 1, "Phase 1 actor must have one UV map: " + id);
        for (const char* field : {"concept", "atlas", "blend", "glb"}) {
            const auto path = root / actor.at(field).get<std::string>();
            require(std::filesystem::exists(path),
                    std::string("missing Phase 1 ") + field + " for " + id);
            require(std::filesystem::file_size(path) > 10000,
                    std::string("Phase 1 ") + field + " is unexpectedly small for " + id);
        }
    }
}

void validateProductionPhaseTwo(const std::filesystem::path& root) {
    const auto manifest = loadJson(root / "production" / "phase2" / "phase2_manifest.json");
    require(manifest.value("schemaVersion", 0) == 1, "Phase 2 schemaVersion must be 1");
    require(manifest.value("status", std::string{}) == "complete", "Phase 2 is not complete");
    require(manifest.value("unitSystem", std::string{}) == "METRIC", "Phase 2 must use metric units");
    require(std::abs(manifest.value("unitMeters", 0.0) - 1.0) < 0.0001,
            "Phase 2 unit scale must be one metre");
    require(manifest.value("originContract", std::string{}) == "body_center_ground_Z0",
            "Phase 2 has the wrong origin contract");
    const auto& actors = manifest.at("actors");
    require(actors.is_array() && actors.size() == 10, "Phase 2 must contain ten actors");
    for (const auto& actor : actors) {
        const std::string id = actor.value("id", std::string{});
        for (const char* field : {"location", "rotationDegrees", "scale", "boundsMeters"}) {
            require(actor.at(field).is_array() && actor.at(field).size() == 3,
                    std::string("invalid Phase 2 ") + field + " for " + id);
        }
        for (double value : actor.at("location").get<std::vector<double>>())
            require(std::abs(value) < 0.00001, "Phase 2 location was not applied for " + id);
        for (double value : actor.at("rotationDegrees").get<std::vector<double>>())
            require(std::abs(value) < 0.00001, "Phase 2 rotation was not applied for " + id);
        for (double value : actor.at("scale").get<std::vector<double>>())
            require(std::abs(value - 1.0) < 0.00001, "Phase 2 scale was not applied for " + id);
        require(std::abs(actor.value("groundMinZ", 1.0)) < 0.00001,
                "Phase 2 actor is not grounded at Z=0: " + id);
        for (double value : actor.at("boundsMeters").get<std::vector<double>>())
            require(value > 0.05 && value < 10.0, "Phase 2 bounds invalid for " + id);
        for (const char* field : {"blend", "glb"}) {
            const auto path = root / actor.at(field).get<std::string>();
            require(std::filesystem::exists(path) && std::filesystem::file_size(path) > 10000,
                    std::string("missing Phase 2 ") + field + " for " + id);
        }
    }
}

void validateProductionPhaseThree(const std::filesystem::path& root) {
    const auto manifest = loadJson(root / "production" / "phase3" / "phase3_manifest.json");
    require(manifest.value("schemaVersion", 0) == 1, "Phase 3 schemaVersion must be 1");
    require(manifest.value("status", std::string{}) == "complete", "Phase 3 is not complete");
    require(manifest.value("rootBone", std::string{}) == "root", "Phase 3 requires a root bone");
    require(!manifest.value("skinned", true), "Phase 3 must precede skinning");
    require(!manifest.value("animated", true), "Phase 3 must precede animation");
    const auto& actors = manifest.at("actors");
    require(actors.is_array() && actors.size() == 10, "Phase 3 must contain ten rigs");
    for (const auto& actor : actors) {
        const std::string id = actor.value("id", std::string{});
        const std::string archetype = actor.value("archetype", std::string{});
        const auto& rootBones = actor.at("rootBones");
        require(rootBones.is_array() && rootBones.size() == 1 && rootBones[0] == "root",
                "Phase 3 rig must have exactly one root: " + id);
        require(!actor.value("skinned", true) && !actor.value("animated", true),
                "Phase 3 rig has work from a later milestone: " + id);
        const auto& bones = actor.at("bones");
        require(bones.is_array() && static_cast<int>(bones.size()) == actor.value("boneCount", 0),
                "Phase 3 bone count mismatch: " + id);
        const int minimumBones = archetype == "floating" ? 8 : 20;
        require(actor.value("boneCount", 0) >= minimumBones, "Phase 3 rig is incomplete: " + id);
        std::set<std::string> names;
        for (const auto& bone : bones) names.insert(bone.value("name", std::string{}));
        require(names.count("root") == 1, "Phase 3 root missing: " + id);
        if (archetype == "humanoid") {
            for (const char* required : {"hips", "spine", "chest", "head", "upper_arm.L",
                                         "upper_arm.R", "thigh.L", "thigh.R", "foot.L", "foot.R"})
                require(names.count(required) == 1, std::string("missing humanoid bone ") + required + " in " + id);
        } else if (archetype == "quadruped" || archetype == "chimera") {
            for (const char* required : {"pelvis", "spine.01", "spine.02", "front_upper.L",
                                         "front_upper.R", "rear_upper.L", "rear_upper.R", "tail.01"})
                require(names.count(required) == 1, std::string("missing quadruped bone ") + required + " in " + id);
        } else {
            for (const char* required : {"body", "eye", "cannon.L", "cannon.R"})
                require(names.count(required) == 1, std::string("missing floating bone ") + required + " in " + id);
        }
        const auto blend = root / actor.at("blend").get<std::string>();
        require(std::filesystem::exists(blend) && std::filesystem::file_size(blend) > 10000,
                "missing Phase 3 Blender rig for " + id);
    }
}

void validateProductionPhaseFour(const std::filesystem::path& root) {
    const auto manifest = loadJson(root / "production" / "phase4" / "phase4_manifest.json");
    require(manifest.value("schemaVersion", 0) == 1, "Phase 4 schemaVersion must be 1");
    require(manifest.value("status", std::string{}) == "complete", "Phase 4 is not complete");
    require(manifest.value("maxInfluencesPerVertex", 0) == 4,
            "Phase 4 must cap skin influences at four");
    require(manifest.value("restPoseExport", false), "Phase 4 must export the rest pose skin");
    const auto render = root / manifest.at("qaRender").get<std::string>();
    require(std::filesystem::exists(render) && std::filesystem::file_size(render) > 100000,
            "Phase 4 pose QA render is missing");
    const auto& actors = manifest.at("actors");
    require(actors.is_array() && actors.size() == 10, "Phase 4 must contain ten skinned actors");
    for (const auto& actor : actors) {
        const std::string id = actor.value("id", std::string{});
        require(actor.value("weightedVertices", 0) > 300, "Phase 4 weighted mesh too small: " + id);
        require(actor.value("unweightedVertices", -1) == 0, "Phase 4 has unweighted vertices: " + id);
        require(actor.value("maximumInfluences", 0) >= 1 && actor.value("maximumInfluences", 5) <= 4,
                "Phase 4 influence cap violated: " + id);
        require(std::abs(actor.value("minimumNormalizedSum", 0.0) - 1.0) < 0.0001,
                "Phase 4 weights are not normalized: " + id);
        require(actor.value("vertexGroups", 0) >= 8, "Phase 4 vertex groups are incomplete: " + id);
        require(actor.value("rigidMeshComponents", 0) >= 10,
                "Phase 4 connected-component skin audit is incomplete: " + id);
        require(actor.value("movedVertices", 0) > 10, "Phase 4 pose did not deform enough vertices: " + id);
        require(actor.value("maxDisplacementMeters", 0.0) > 0.05,
                "Phase 4 pose produced no meaningful deformation: " + id);
        for (const char* field : {"blend", "glb"}) {
            const auto path = root / actor.at(field).get<std::string>();
            require(std::filesystem::exists(path) && std::filesystem::file_size(path) > 10000,
                    std::string("missing Phase 4 ") + field + " for " + id);
        }
    }
}

void validateProductionControlRigs(const std::filesystem::path& root) {
    const auto manifest = loadJson(root / "production" / "phase5_6" / "phase5_6_manifest.json");
    require(manifest.value("schemaVersion", 0) == 1, "Phase 5-6 schemaVersion must be 1");
    require(manifest.value("status", std::string{}) == "complete", "Phase 5-6 is not complete");
    require(!manifest.value("controlBonesDeform", true), "control bones must never deform the mesh");
    require(manifest.value("ikFkProperty", std::string{}) == "ik_fk", "IK/FK property is missing");
    require(!manifest.value("animated", true), "Phase 5-6 must precede animation Actions");
    const auto render = root / manifest.at("qaRender").get<std::string>();
    require(std::filesystem::exists(render) && std::filesystem::file_size(render) > 100000,
            "Phase 5-6 IK pose render is missing");
    const auto& actors = manifest.at("actors");
    require(actors.is_array() && actors.size() == 10, "Phase 5-6 must contain ten control rigs");
    for (const auto& actor : actors) {
        const std::string id = actor.value("id", std::string{});
        const std::string archetype = actor.value("archetype", std::string{});
        const int expectedControls = archetype == "floating" ? 5 : 9;
        const int expectedConstraints = archetype == "floating" ? 3 : 4;
        const int expectedSwitches = archetype == "floating" ? 0 : 4;
        require(actor.value("controlBoneCount", 0) == expectedControls,
                "wrong Phase 5-6 controller count: " + id);
        require(actor.value("constraintCount", 0) == expectedConstraints,
                "wrong Phase 5-6 constraint count: " + id);
        require(actor.value("ikFkSwitchCount", -1) == expectedSwitches,
                "wrong Phase 5-6 IK/FK switch count: " + id);
        require(!actor.value("animated", true), "Phase 5-6 actor has premature animation: " + id);
        const auto controls = actor.at("controlBones").get<std::vector<std::string>>();
        require(std::find(controls.begin(), controls.end(), "CTRL_master") != controls.end(),
                "Phase 5-6 master controller missing: " + id);
        const auto blend = root / actor.at("blend").get<std::string>();
        require(std::filesystem::exists(blend) && std::filesystem::file_size(blend) > 10000,
                "missing Phase 5-6 Blender control rig for " + id);
    }
}

void validateCombatVfxAssets(const std::filesystem::path& root) {
    const auto manifestPath = root / "textures" / "vfx" / "vfx_assets_manifest.json";
    const auto manifest = loadJson(manifestPath);
    require(manifest.value("schemaVersion", 0) == 1,
            "unsupported combat VFX manifest schema");
    require(manifest.value("status", std::string{}) == "production",
            "combat VFX assets are not marked production-ready");
    require(manifest.value("runtime", std::string{}) == "C++17/raylib",
            "combat VFX manifest must target the C++ runtime");
    const auto& rendering = manifest.at("rendering");
    require(rendering.value("space", std::string{}) == "world",
            "combat VFX must be rendered in 3D world space");
    require(!rendering.value("staticScreenImages", true),
            "combat VFX must not be implemented as static screen images");
    require(rendering.at("techniques").is_array()
                && rendering.at("techniques").size() >= 4,
            "combat VFX must declare its animated world-space techniques");

    const std::map<std::string, std::string> expectedActivePaths{
        {"hero_dash_streak", "textures/vfx/hero_dash_streak_v1.png"},
        {"knight_blade_storm", "textures/vfx/knight_blade_storm_v1.png"},
        {"knight_guard_crest", "textures/vfx/knight_guard_crest_v1.png"},
        {"knight_shield_rush_impact", "textures/vfx/knight_shield_rush_impact_v1.png"},
        {"knight_sword_arc", "textures/vfx/knight_sword_arc_v2.png"},
        {"mage_arcane_bolt", "textures/vfx/mage_arcane_bolt_v1.png"},
        {"mage_arcane_sigil", "textures/vfx/mage_arcane_sigil_v1.png"},
        {"mage_astral_tempest", "textures/vfx/mage_astral_tempest_v1.png"},
        {"mage_frost_nova", "textures/vfx/mage_frost_nova_v1.png"},
        {"mage_gravity_vortex", "textures/vfx/mage_gravity_vortex_v1.png"}
    };
    const std::map<std::string, std::set<std::string>> requiredUsage{
        {"hero_dash_streak", {"knight.dash.active", "mage.dash.active"}},
        {"knight_blade_storm", {"knight.ultimate.active"}},
        {"knight_guard_crest", {"knight.skillOne.active"}},
        {"knight_shield_rush_impact", {"knight.skillTwo.contact"}},
        {"knight_sword_arc", {"knight.basic.contact"}},
        {"mage_arcane_bolt", {"mage.basic.projectile"}},
        {"mage_arcane_sigil", {"mage.ultimate.cast"}},
        {"mage_astral_tempest", {"mage.ultimate.active"}},
        {"mage_frost_nova", {"mage.skillOne.contact"}},
        {"mage_gravity_vortex", {"mage.skillTwo.active"}}
    };
    const std::array<const char*, 10> combatActions{
        "knight.basic", "knight.skillOne", "knight.skillTwo", "knight.ultimate",
        "knight.dash", "mage.basic", "mage.skillOne", "mage.skillTwo",
        "mage.ultimate", "mage.dash"
    };
    const std::array<const char*, 8> primaryCombatActions{
        "knight.basic", "knight.skillOne", "knight.skillTwo", "knight.ultimate",
        "mage.basic", "mage.skillOne", "mage.skillTwo", "mage.ultimate"
    };
    const auto& assets = manifest.at("assets");
    require(assets.is_array() && assets.size() == expectedActivePaths.size() + 1,
            "combat VFX manifest must contain ten active assets and one legacy asset");
    require(manifest.value("activeAssetCount", 0)
                == static_cast<int>(expectedActivePaths.size()),
            "combat VFX activeAssetCount must be ten");
    constexpr std::array<unsigned char, 8> pngSignature{
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
    };
    const auto readBigEndian = [](const std::array<unsigned char, 33>& bytes,
                                  std::size_t offset) {
        return (static_cast<std::uint32_t>(bytes[offset]) << 24u)
             | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16u)
             | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8u)
             | static_cast<std::uint32_t>(bytes[offset + 3]);
    };

    std::set<std::string> seenIds;
    std::set<std::string> seenPaths;
    std::set<std::string> seenActiveIds;
    std::set<std::string> coveredActions;
    std::set<std::string> primaryActions;
    int supersededCount = 0;
    for (const auto& asset : assets) {
        const std::string id = asset.value("id", std::string{});
        const std::string relativePath = asset.value("path", std::string{});
        const std::string status = asset.value("status", std::string{});
        require(!id.empty() && seenIds.insert(id).second,
                "duplicate or empty combat VFX asset id " + id);
        require(!relativePath.empty() && seenPaths.insert(relativePath).second,
                "duplicate or empty combat VFX asset path " + relativePath);
        require(asset.value("alpha", false),
                "combat VFX texture must preserve transparency");
        const auto& dimensions = asset.at("dimensions");
        require(dimensions.is_array() && dimensions.size() == 2
                    && dimensions[0].get<int>() >= 512
                    && dimensions[1].get<int>() >= 512,
                "combat VFX texture resolution is too small");

        const auto path = root / relativePath;
        require(std::filesystem::exists(path)
                    && std::filesystem::file_size(path) > 50000,
                "missing or unexpectedly small combat VFX texture " + path.string());
        std::ifstream input(path, std::ios::binary);
        std::array<unsigned char, 33> header{};
        input.read(reinterpret_cast<char*>(header.data()), header.size());
        require(static_cast<bool>(input)
                    && std::equal(pngSignature.begin(), pngSignature.end(), header.begin()),
                "invalid PNG combat VFX texture " + path.string());
        require(readBigEndian(header, 8) == 13u
                    && header[12] == 'I' && header[13] == 'H'
                    && header[14] == 'D' && header[15] == 'R',
                "combat VFX PNG has no valid IHDR " + path.string());
        const int pngWidth = static_cast<int>(readBigEndian(header, 16));
        const int pngHeight = static_cast<int>(readBigEndian(header, 20));
        require(pngWidth == dimensions[0].get<int>()
                    && pngHeight == dimensions[1].get<int>(),
                "combat VFX manifest dimensions do not match PNG IHDR " + path.string());
        const unsigned char colorType = header[25];
        require(colorType == 4u || colorType == 6u,
                "combat VFX PNG must have an alpha-bearing grayscale/RGBA color type "
                    + path.string());
        if (id == "knight_sword_arc" || id == "knight_shield_rush_impact") {
            require(pngWidth >= pngHeight * 5 / 2,
                    "directional Knight VFX must stay wide enough for its "
                    "world-space blade/rush trajectory");
        }

        const auto& usage = asset.at("usage");
        require(usage.is_array(), "combat VFX usage must be an array for " + id);
        std::set<std::string> actualUsage;
        for (const auto& value : usage) {
            require(value.is_string() && !value.get<std::string>().empty(),
                    "combat VFX usage contains an empty value for " + id);
            actualUsage.insert(value.get<std::string>());
        }

        if (status == "active") {
            const auto expectedPath = expectedActivePaths.find(id);
            require(expectedPath != expectedActivePaths.end(),
                    "unexpected active combat VFX asset " + id);
            require(relativePath == expectedPath->second,
                    "wrong path for active combat VFX asset " + id);
            require(seenActiveIds.insert(id).second,
                    "duplicate active combat VFX asset " + id);
            require(!actualUsage.empty(), "active combat VFX asset has no usage " + id);
            if (asset.contains("primaryAction")) {
                const std::string primary = asset.at("primaryAction").get<std::string>();
                require(primaryActions.insert(primary).second,
                        "multiple VFX assets claim primary action " + primary);
            }
            const auto requirements = requiredUsage.find(id);
            require(requirements != requiredUsage.end(),
                    "active combat VFX asset has no usage contract " + id);
            for (const auto& required : requirements->second) {
                require(actualUsage.count(required) == 1,
                        "active combat VFX asset " + id
                            + " is missing usage " + required);
            }
            for (const auto& action : combatActions) {
                const std::string prefix = std::string(action) + ".";
                if (std::any_of(actualUsage.begin(), actualUsage.end(),
                                [&prefix](const std::string& value) {
                                    return value.rfind(prefix, 0) == 0;
                                })) {
                    coveredActions.insert(action);
                }
            }
        } else if (status == "superseded") {
            ++supersededCount;
            require(id == "knight_violet_slash"
                        && relativePath == "textures/vfx/knight_violet_slash_v1.png",
                    "only the original Knight violet slash may remain as legacy VFX");
            require(asset.value("supersededBy", std::string{}) == "knight_sword_arc",
                    "legacy Knight slash must point to its directional replacement");
            require(actualUsage.empty(),
                    "superseded combat VFX must not retain runtime usage");
        } else {
            require(false, "invalid combat VFX lifecycle status for " + id);
        }
    }
    require(seenActiveIds.size() == expectedActivePaths.size(),
            "one or more required active combat VFX assets are missing");
    require(supersededCount == 1,
            "combat VFX manifest must retain exactly one superseded legacy asset");
    require(coveredActions.size() == combatActions.size(),
            "every Knight and Mage combat action must have active texture coverage");
    require(primaryActions.size() == primaryCombatActions.size(),
            "all three regular skills and the Ultimate of both heroes need unique VFX");
    for (const char* action : primaryCombatActions)
        require(primaryActions.count(action) == 1,
                std::string("missing unique primary VFX for ") + action);
}

void validateAegisRiftEnvironment(const std::filesystem::path& root) {
    const auto model = root / "models" / "environment" / "aegis_rift_arena_v1.glb";
    const auto source = root / "source" / "blender" / "environment"
                      / "aegis_rift_arena_v1.blend";
    const auto backdrop = root / "textures" / "environment"
                        / "aegis_rift_void_panorama_v1.png";
    const auto qa = root / "production_v3" / "qa" / "environment"
                  / "aegis_rift_arena_v1" / "qa_three_quarter.png";
    require(std::filesystem::exists(model)
                && std::filesystem::file_size(model) > 100000,
            "missing production Aegis Rift environment GLB");
    require(std::filesystem::exists(source)
                && std::filesystem::file_size(source) > 500000,
            "missing editable Aegis Rift Blender source");
    require(std::filesystem::exists(backdrop)
                && std::filesystem::file_size(backdrop) > 500000,
            "missing animated-void panorama source texture");
    require(std::filesystem::exists(qa)
                && std::filesystem::file_size(qa) > 50000,
            "missing Aegis Rift environment QA render");

    const auto gltf = loadGlbJsonChunk(model);
    require(gltf.at("asset").value("version", std::string{}) == "2.0",
            "Aegis Rift environment must use glTF 2.0");
    require(gltf.contains("meshes") && gltf.at("meshes").is_array()
                && !gltf.at("meshes").empty(),
            "Aegis Rift environment has no mesh");
    require(gltf.contains("materials") && gltf.at("materials").is_array()
                && gltf.at("materials").size() >= 6,
            "Aegis Rift environment lost its authored material palette");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: Survival3DConfigTests <assets/survival3d>");
        const std::filesystem::path root = argv[1];
        validateWaveTable(root);
        std::cout << "[PASS] 50-wave table and boss schedule\n";
        validateScaling(root);
        std::cout << "[PASS] monotonic scaling and hard caps\n";
        validateServiceSafety(root);
        std::cout << "[PASS] offline-safe service config\n";
        validateCharacterConcepts(root);
        std::cout << "[PASS] character turnaround assets\n";
        validateAnimatedRoster(root);
        std::cout << "[PASS] 10-model skeletal animation roster\n";
        validateStepSevenAssets(root);
        std::cout << "[PASS] Step 7: 104 named actor actions and ten required animated 3D skill assets\n";
        validateProductionPhaseOne(root);
        std::cout << "[PASS] Phase 1 production mesh/material checkpoint\n";
        validateProductionPhaseTwo(root);
        std::cout << "[PASS] Phase 2 transforms, origin and axis checkpoint\n";
        validateProductionPhaseThree(root);
        std::cout << "[PASS] Phase 3 archetype deform-skeleton checkpoint\n";
        validateProductionPhaseFour(root);
        std::cout << "[PASS] Phase 4 normalized skinning and deformation checkpoint\n";
        validateProductionControlRigs(root);
        std::cout << "[PASS] Phase 5-6 IK/FK production control-rig checkpoint\n";
        validateCombatVfxAssets(root);
        std::cout << "[PASS] Step 6 eight active Knight/Mage combat VFX textures"
                     " plus one superseded legacy texture\n";
        validateAegisRiftEnvironment(root);
        std::cout << "[PASS] M7-M8 Aegis Rift arena, source, QA and void panorama\n";
        std::cout << "All Survival3D config tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}

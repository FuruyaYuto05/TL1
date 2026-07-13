#include "LevelLoader.h"

#include <fstream>
#include <cassert>

#include "externals/json.hpp"

using json = nlohmann::json;

const std::string kDefaultBaseDirectory = "resources/";
const std::string kExtension = ".json";

// オブジェクト1個分を読み込む
void ParseObject(const json& object, LevelData* levelData)
{
    assert(object.is_object());
    assert(object.contains("type"));

    std::string type = object["type"].get<std::string>();

    if (type.compare("MESH") == 0) {

        levelData->objects.emplace_back(LevelData::ObjectData{});
        LevelData::ObjectData& objectData = levelData->objects.back();

        objectData.type = type;

        if (object.contains("name")) {
            objectData.name = object["name"].get<std::string>();
        }

        if (object.contains("file_name")) {
            objectData.fileName = object["file_name"].get<std::string>();
        }

        assert(object.contains("transform"));

        const json& transform = object["transform"];

        objectData.translation.x = (float)transform["translation"][0];
        objectData.translation.y = (float)transform["translation"][2];
        objectData.translation.z = (float)transform["translation"][1];

        objectData.rotation.x = (float)transform["rotation"][0];
        objectData.rotation.y = (float)transform["rotation"][2];
        objectData.rotation.z = (float)transform["rotation"][1];

        objectData.scaling.x = (float)transform["scaling"][0];
        objectData.scaling.y = (float)transform["scaling"][2];
        objectData.scaling.z = (float)transform["scaling"][1];
    }

    if (object.contains("children")) {
        assert(object["children"].is_array());

        for (const json& child : object["children"]) {
            ParseObject(child, levelData);
        }
    }
}

LevelData* LoadLevelData(const std::string& fileName)
{
    const std::string fullpath = kDefaultBaseDirectory + fileName + kExtension;

    std::ifstream file;
    file.open(fullpath);

    if (file.fail()) {
        assert(0);
    }

    json deserialized;
    file >> deserialized;

    assert(deserialized.is_object());
    assert(deserialized.contains("name"));
    assert(deserialized["name"].is_string());

    std::string name = deserialized["name"].get<std::string>();
    assert(name.compare("scene") == 0);

    assert(deserialized.contains("objects"));
    assert(deserialized["objects"].is_array());

    LevelData* levelData = new LevelData();

    for (const json& object : deserialized["objects"]) {
        ParseObject(object, levelData);
    }

    return levelData;
}
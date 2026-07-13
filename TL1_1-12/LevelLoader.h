#pragma once

#include <string>
#include <vector>

struct Vector3 {
    float x;
    float y;
    float z;
};

struct LevelData {

    struct ObjectData {
        std::string type;
        std::string name;
        std::string fileName;

        Vector3 translation;
        Vector3 rotation;
        Vector3 scaling;
    };

    std::vector<ObjectData> objects;
};

LevelData* LoadLevelData(const std::string& fileName);
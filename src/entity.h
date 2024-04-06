
#ifndef WEBSERVER_ENTITY_H
#define WEBSERVER_ENTITY_H

#include <jsoncpp/json/json.h>
class Entity {
public:
    Entity() {}
    Entity(const int id, const std::string& name, const std::string& password)
            : id(id), name(name), password(password) {}

    // 从 JSON 对象反序列化
    bool deserialize(const Json::Value& json) {
        if (!json.isObject()) {
            return false;
        }

        id = json.get("id", "").asInt();
        name = json.get("name", "").asString();
        password = json.get("password", "").asString();
        return true;
    }

    // 序列化到 JSON 对象
    Json::Value serialize() const {
        Json::Value json;
        json["id"] = id;
        json["name"] = name;
        json["password"] = password;
        return json;
    }

    // 省略其他成员函数和数据成员的访问器

private:
    int id;
    std::string name;
    std::string password;

};


#endif //WEBSERVER_ENTITY_H

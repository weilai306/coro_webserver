#ifndef WEBSERVER_RESTFULPARSER_H
#define WEBSERVER_RESTFULPARSER_H
#include <string>
#include <unordered_map>
#include <vector>
#include "entity.h"
#include <memory>
class RestfulParser {

public:
    RestfulParser(){
        str2entity_ = {{"entity", std::make_shared<Entity>()}};
    }
    std::pair<std::shared_ptr<Entity>, std::string> parse(std::string& url) const{
        if(url.empty() || url[0] != '/' || url.size() == 1){
            return {nullptr,"index.html"};
        }
        auto last = url.find_last_of('/');
        auto substr = url.substr(1,last-1);
        auto ite = str2entity_.find(substr);
        std::shared_ptr<Entity> entity_ptr;
        if(ite != str2entity_.end()){
            entity_ptr = ite->second;
        }else{
            entity_ptr = nullptr;
        }

        std::string resource = url.substr(last+1,url.size()-last);
        return {entity_ptr, resource};
    }
private:
    std::unordered_map<std::string, std::shared_ptr<Entity>> str2entity_;
};


#endif //WEBSERVER_RESTFULPARSER_H

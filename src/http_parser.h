#ifndef WEBSERVER_HTTPPARSER_H
#define WEBSERVER_HTTPPARSER_H
#include <http_parser.h>
#include <iostream>
#include <cstring>
#include <functional>

class HttpParser {
public:
    http_parser httpParser;
    http_parser_settings httpSettings;
    std::string url_;
    std::string body_;
    enum http_method method_;
    std::string getUrl(){
        return std::move(url_);
    }

    std::string getBody(){
        return std::move(body_);
    }

    Task<bool> parse(char* data, size_t length){
        bzero(&httpSettings,sizeof(httpSettings));
        http_parser_init(&httpParser, HTTP_REQUEST);
        http_parser_settings_init(&httpSettings);
        httpParser.data = this;
        httpSettings.on_url = [](http_parser* parser, const char *at, size_t length){
            auto p = (HttpParser*)parser->data;
            p->url_ = std::string(at,length);
            return 0;
        };

        httpSettings.on_body = [](http_parser*parser, const char *at, size_t length){
            auto p = (HttpParser*)parser->data;
            p->body_ = std::string(at,length);
            return 0;
        };
        do{
            // 解析请求
            size_t nParseBytes = http_parser_execute(&httpParser, &httpSettings, data, length);
            method_ = (enum http_method)httpParser.method;
            if(nParseBytes != length){
                co_return false;
            }
            co_return true;
        }while(false);
    }
};


#endif //WEBSERVER_HTTPPARSER_H

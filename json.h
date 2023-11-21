#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <unordered_map>

#define OK 0
#define ERROR -1
#define INVALID -2
#define AGAIN -3

enum class JsonType
{
    UNDEFINED,
    OBJECT,
    ARRAY,
    STRING,
    PRIMITIVE
};

struct Token
{
    JsonType type;
    int start;
    int end;
    int size;
};

struct Parser
{
    unsigned int pos;
    unsigned int nextToken;
    int preToken;
};

struct JsonObject
{
    std::string key;
    Token value;
    std::unordered_map<std::string, JsonObject> objectMap;
};

class JsonParser
{
  public:
    JsonParser(size_t maxToken = 1024);
    ~JsonParser();

    Token *allocToken();
    void setToken(Token *token, const JsonType type, const int start, const int end);
    int parsePrimitive(const char *js, const size_t len);
    int parseString(const char *js, const size_t len);
    int doParse(const char *js, const size_t len);
    void parseToken();

    Parser parser_;

    Token *tokens_;
    size_t size_;

    JsonObject object_;
    size_t idx_;
};

#endif
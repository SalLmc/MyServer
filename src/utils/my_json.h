#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <unordered_map>
#include <vector>

#define OK 0
#define ERROR -1
#define INVALID -2
#define PART -3

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
    std::string toString(const char *json);
    JsonType type;
    int start;
    int end;
    int size;

    // used in extract value
    int idx;
};

struct Parser
{
    int pos;
    int nextIdx;
    int fatherTokenIdx;
};

class JsonParser
{
  public: 
    JsonParser(const char *json, int len, int maxToken = 1024);
    ~JsonParser();

    Token *get(const char *key, Token *root);
    Token *get(int arrIdx, Token *root);

    Token *allocToken();
    void setToken(Token *token, const JsonType type, const int start, const int end);
    int parsePrimitive();
    int parseString();
    int doParse();

    // @return idx of the next non-related token
    int skipToken(Token *token);

    Parser parser_;

    const char *json_;
    int len_;

    std::vector<Token> tokens_;
    int maxSize_;
};

#endif
#ifndef JSON_HPP
#define JSON_HPP

#include "../headers.h"

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
    Parser();
    int pos;
    int nextIdx;
    int fatherTokenIdx;
};

class JsonResult
{
  public:
    JsonResult(const char *json, int len, std::vector<Token> *tokens);
    JsonResult get(const char *key);
    JsonResult get(int arrIdx);
    Token *value();

  private:
    // @return idx of the next non-related token
    int skipToken(Token *token);

    const char *json_;
    int len_;
    std::vector<Token> *tokens_;
    Token *now_;
};

class JsonParser
{
  public:
    JsonParser(std::vector<Token> *tokens, const char *json, int len, int maxToken = 1024);
    ~JsonParser();

    JsonResult parse();

  private:
    Token *allocToken();
    void setToken(Token *token, const JsonType type, const int start, const int end);
    int parsePrimitive();
    int parseString();
    int doParse();

    Parser parser_;

    const char *json_;
    int len_;

    std::vector<Token> *tokens_;
};

#endif
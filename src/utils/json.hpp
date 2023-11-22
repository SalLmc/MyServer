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
    std::string toString(const char *json)
    {
        return std::string(json + start, json + end);
    }
    JsonType type;
    int start;
    int end;
    int size;

    // used in extract value
    int idx;
};

struct Parser
{
    Parser() : pos(0), nextIdx(0), fatherTokenIdx(-1)
    {
    }
    int pos;
    int nextIdx;
    int fatherTokenIdx;
};

class JsonResult
{
  public:
    JsonResult(const char *json, int len, std::vector<Token> *tokens, int tokenSize)
        : json_(json), len_(len), tokens_(tokens), tokenSize_(tokenSize), now_(&tokens_->at(0))
    {
    }
    JsonResult operator[](const char *key);
    JsonResult operator[](int arrIdx);

    JsonResult get(const char *key);
    JsonResult get(int arrIdx);

    int size();

    Token *raw();

    template <typename T> T value();

  private:
    // @return idx of the next non-related token
    int skipToken(Token *token);

    const char *json_;
    int len_;

    std::vector<Token> *tokens_;
    int tokenSize_;

    Token *now_;
};

class JsonParser
{
  public:
    JsonParser(std::vector<Token> *tokens, const char *json, int len)
        : json_(json), len_(len), tokens_(tokens), tokenSize_(0)
    {
    }

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
    int tokenSize_;
};

int JsonResult::size()
{
    return now_->size;
}

Token *JsonParser::allocToken()
{
    Token *tok;
    if ((size_t)parser_.nextIdx >= tokens_->size())
    {
        return NULL;
    }
    tok = &tokens_->at(parser_.nextIdx);
    tok->idx = parser_.nextIdx++;
    tok->start = tok->end = -1;
    tok->size = 0;
    return tok;
}

void JsonParser::setToken(Token *token, const JsonType type, const int start, const int end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

int JsonParser::parsePrimitive()
{
    Token *token;
    int start = parser_.pos;
    for (; parser_.pos < len_ && json_[parser_.pos] != '\0'; parser_.pos++)
    {
        switch (json_[parser_.pos])
        {
        case ':':
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
        case ']':
        case '}':
            goto found;
        default:
            break;
        }

        if (json_[parser_.pos] < 32 || json_[parser_.pos] >= 127)
        {
            parser_.pos = start;
            return INVALID;
        }
    }

found:

    token = allocToken();

    if (token == NULL)
    {
        parser_.pos = start;
        return ERROR;
    }

    setToken(token, JsonType::PRIMITIVE, start, parser_.pos);
    parser_.pos--;

    return OK;
}

int JsonParser::parseString()
{
    Token *token;

    int start = parser_.pos;

    parser_.pos++;

    for (; parser_.pos < len_ && json_[parser_.pos] != '\0'; parser_.pos++)
    {
        char c = json_[parser_.pos];

        if (c == '\"')
        {
            token = allocToken();

            if (token == NULL)
            {
                parser_.pos = start;
                return ERROR;
            }

            setToken(token, JsonType::STRING, start + 1, parser_.pos);

            return OK;
        }

        if (c == '\\' && parser_.pos + 1 < len_)
        {
            parser_.pos++;
            switch (json_[parser_.pos])
            {
            case '\"':
            case '/':
            case '\\':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
                break;
            // handle things like '\uXXXX'
            case 'u':
                parser_.pos++;
                for (int i = 0; i < 4 && parser_.pos < len_ && json_[parser_.pos] != '\0'; i++)
                {
                    if (!((json_[parser_.pos] >= 48 && json_[parser_.pos] <= 57) ||
                          (json_[parser_.pos] >= 65 && json_[parser_.pos] <= 70) ||
                          (json_[parser_.pos] >= 97 && json_[parser_.pos] <= 102)))
                    {
                        parser_.pos = start;
                        return INVALID;
                    }
                    parser_.pos++;
                }
                parser_.pos--;
                break;
            default:
                parser_.pos = start;
                return INVALID;
            }
        }
    }

    parser_.pos = start;

    return PART;
}

int JsonParser::doParse()
{
    int i, rc;
    JsonType type;
    Token *token;
    int cnt = parser_.nextIdx;

    for (; parser_.pos < len_ && json_[parser_.pos] != '\0'; parser_.pos++)
    {
        char c = json_[parser_.pos];

        switch (c)
        {
        case '{':
        case '[':

            cnt++;

            token = allocToken();

            if (token == NULL)
            {
                return ERROR;
            }

            if (parser_.fatherTokenIdx != -1)
            {
                Token *tok = &tokens_->at(parser_.fatherTokenIdx);
                tok->size++;
            }

            token->type = (c == '{' ? JsonType::OBJECT : JsonType::ARRAY);
            token->start = parser_.pos;

            // set fatherIdx to current token, Since we need to connect attributes of this object to this token
            parser_.fatherTokenIdx = parser_.nextIdx - 1;

            break;

        case '}':
        case ']':

            type = (c == '}' ? JsonType::OBJECT : JsonType::ARRAY);

            // find the token of current object
            for (i = parser_.nextIdx - 1; i >= 0; i--)
            {
                token = &tokens_->at(i);

                // found
                if (token->start != -1 && token->end == -1)
                {
                    if (token->type != type)
                    {
                        return INVALID;
                    }

                    // clear fatherIdx, since an object has been parsed
                    parser_.fatherTokenIdx = -1;

                    token->end = parser_.pos + 1;

                    break;
                }
            }

            if (i == -1)
            {
                return INVALID;
            }

            // set fatherIdx to previous idx (handle recursive situations)
            for (; i >= 0; i--)
            {
                token = &tokens_->at(i);
                if (token->start != -1 && token->end == -1)
                {
                    parser_.fatherTokenIdx = i;
                    break;
                }
            }

            break;

        case '\"':

            rc = parseString();
            if (rc < 0)
            {
                return rc;
            }

            cnt++;

            if (parser_.fatherTokenIdx != -1)
            {
                tokens_->at(parser_.fatherTokenIdx).size++;
            }

            break;

        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            parser_.fatherTokenIdx = parser_.nextIdx - 1;
            break;

        case ',':

            if (parser_.fatherTokenIdx != -1 && tokens_->at(parser_.fatherTokenIdx).type != JsonType::ARRAY &&
                tokens_->at(parser_.fatherTokenIdx).type != JsonType::OBJECT)
            {
                for (i = parser_.nextIdx - 1; i >= 0; i--)
                {
                    if (tokens_->at(i).type == JsonType::ARRAY || tokens_->at(i).type == JsonType::OBJECT)
                    {
                        if (tokens_->at(i).start != -1 && tokens_->at(i).end == -1)
                        {
                            parser_.fatherTokenIdx = i;
                            break;
                        }
                    }
                }
            }
            break;

        default:

            rc = parsePrimitive();
            if (rc < 0)
            {
                return rc;
            }

            cnt++;

            if (parser_.fatherTokenIdx != -1)
            {
                tokens_->at(parser_.fatherTokenIdx).size++;
            }

            break;
        }
    }

    // check whether all tokens have been fully parsed
    for (i = parser_.nextIdx - 1; i >= 0; i--)
    {
        if (tokens_->at(i).start != -1 && tokens_->at(i).end == -1)
        {
            return PART;
        }
    }

    return cnt;
}

JsonResult JsonParser::parse()
{
    int rc = doParse();
    if (rc < 0)
    {
        throw std::runtime_error("parse failed");
    }
    tokenSize_ = rc;

    return JsonResult(json_, len_, tokens_, rc);
}

JsonResult JsonResult::get(const char *key)
{
    if (now_->type != JsonType::OBJECT)
    {
        throw std::runtime_error("no such key");
    }

    int size = now_->size;
    int idx = now_->idx + 1;

    for (int i = 0; i < size; i++)
    {
        if (tokens_->at(idx).toString(json_) == key)
        {
            JsonResult ans = *this;
            ans.now_ = &tokens_->at(idx + 1);
            return ans;
        }

        // skip this object
        idx = skipToken(&tokens_->at(idx + 1));
    }

    throw std::runtime_error("no such key");
}

JsonResult JsonResult::get(int arrIdx)
{
    if (now_->type != JsonType::ARRAY || arrIdx >= now_->size)
    {
        throw std::runtime_error("invalid idx");
    }

    int idx = now_->idx + 1;

    for (int i = 0; i < arrIdx; i++)
    {
        // skip this object
        idx = skipToken(&tokens_->at(idx));
    }

    JsonResult ans = *this;
    ans.now_ = &tokens_->at(idx);
    return ans;
}

JsonResult JsonResult::operator[](const char *key)
{
    return get(key);
}

JsonResult JsonResult::operator[](int arrIdx)
{
    return get(arrIdx);
}

Token *JsonResult::raw()
{
    return now_;
}

int JsonResult::skipToken(Token *token)
{
    int idx = token->idx;
    int size = token->size;

    switch (token->type)
    {
    case JsonType::OBJECT:
        // points to the first inner value
        idx += 2;
        for (int i = 0; i < size; i++)
        {
            idx = skipToken(&tokens_->at(idx)) + 1;
        }
        idx--;
        break;
    case JsonType::ARRAY:
        // points to the first inner value
        idx++;
        for (int i = 0; i < size; i++)
        {
            idx = skipToken(&tokens_->at(idx));
        }
        break;
    default:
        idx++;
        break;
    }

    return idx;
}

template <> bool JsonResult::value()
{
    Token *tok = now_;

    if (tok->type != JsonType::PRIMITIVE)
    {
        throw std::runtime_error("wrong type");
    }

    std::string s(json_ + tok->start, json_ + tok->end);

    if (s == "true" || s == "1")
    {
        return true;
    }
    else if (s == "false" || s == "0")
    {
        return false;
    }

    throw std::runtime_error("bool value error");
}

template <> int JsonResult::value()
{
    Token *tok = now_;

    if (tok->type != JsonType::PRIMITIVE)
    {
        throw std::runtime_error("wrong type");
    }

    std::string s(json_ + tok->start, json_ + tok->end);
    int ans = std::stoi(s);

    return ans;
}

template <> std::string JsonResult::value()
{
    Token *tok = now_;

    if (tok->type != JsonType::STRING)
    {
        throw std::runtime_error("wrong type");
    }

    return std::string(json_ + tok->start, json_ + tok->end);
}

template <> std::vector<std::string> JsonResult::value()
{
    Token *tok = now_;

    if (tok->type != JsonType::ARRAY)
    {
        throw std::runtime_error("wrong type");
    }

    int size = tok->size;
    int idx = tok->idx + 1;
    now_ = &tokens_->at(idx);

    std::vector<std::string> ans;

    for (int i = 0; i < size; i++)
    {
        ans.push_back(value<std::string>());
        idx = skipToken(now_);
        now_ = &tokens_->at(idx);
    }

    return ans;
}

template <> std::unordered_map<std::string, std::string> JsonResult::value()
{
    Token *tok = now_;

    if (tok->type != JsonType::OBJECT)
    {
        throw std::runtime_error("wrong type");
    }

    int size = tok->size;
    int idx = tok->idx + 1;
    now_ = &tokens_->at(idx);

    std::unordered_map<std::string, std::string> ans;

    for (int i = 0; i < size; i++)
    {
        std::string key = value<std::string>();
        now_ = &tokens_->at(++idx);
        std::string val = value<std::string>();

        ans.insert({key, val});
        idx = skipToken(now_);
        now_ = &tokens_->at(idx);
    }

    return ans;
}

template <typename T> T getValue(JsonResult &json, const std::string &key, T defaultValue)
{
    try
    {
        auto val = json[key.c_str()].value<T>();
        return val;
    }
    catch (const std::exception &e)
    {
    }
    return defaultValue;
}

#endif
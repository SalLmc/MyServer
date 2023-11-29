#ifndef JSON_HPP
#define JSON_HPP

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#define JSON_OK 0
#define JSON_ERROR -1
#define JSON_INVALID -2
#define JSON_INCOMPLETE -3

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

class JsonResult
{
  public:
    JsonResult(const char *json, int len, std::vector<Token> *tokens)
        : json_(json), len_(len), tokens_(tokens), now_(&tokens_->at(0))
    {
    }
    JsonResult operator[](const char *key);
    JsonResult operator[](int arrIdx);

    JsonResult get(const char *key);
    JsonResult get(int arrIdx);

    int size();

    Token *raw();

    template <typename T> T value();
    template <typename T> T value(T defaultValue);

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
    JsonParser(const char *json, int len) : json_(json), len_(len), pos_(0)
    {
    }

    JsonResult parse();

  private:
    Token *allocToken();
    void setToken(Token *token, const JsonType type, const int start, const int end);
    int parsePrimitive();
    int parseString();
    // @return real size of tokens
    int doParse();

    const char *json_;
    int len_;

    int pos_;

    std::stack<int> fatherIdx_;

    std::vector<Token> tokens_;
};

int JsonResult::size()
{
    return now_->size;
}

Token *JsonParser::allocToken()
{
    tokens_.emplace_back();
    Token *tok = &tokens_.back();
    tok->idx = tokens_.size() - 1;
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
    int start = pos_;
    for (; pos_ < len_ && json_[pos_] != '\0'; pos_++)
    {
        switch (json_[pos_])
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

        if (json_[pos_] < 32 || json_[pos_] >= 127)
        {
            pos_ = start;
            return JSON_INVALID;
        }
    }

found:

    token = allocToken();

    if (token == NULL)
    {
        pos_ = start;
        return JSON_ERROR;
    }

    setToken(token, JsonType::PRIMITIVE, start, pos_);
    pos_--;

    return JSON_OK;
}

int JsonParser::parseString()
{
    Token *token;

    int start = pos_;

    pos_++;

    for (; pos_ < len_ && json_[pos_] != '\0'; pos_++)
    {
        char c = json_[pos_];

        if (c == '\"')
        {
            token = allocToken();

            if (token == NULL)
            {
                pos_ = start;
                return JSON_ERROR;
            }

            setToken(token, JsonType::STRING, start + 1, pos_);

            return JSON_OK;
        }

        // handle escape characters
        if (c == '\\' && pos_ + 1 < len_)
        {
            pos_++;
            switch (json_[pos_])
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
                pos_++;
                for (int i = 0; i < 4 && pos_ < len_ && json_[pos_] != '\0'; i++)
                {
                    if (!((json_[pos_] >= 48 && json_[pos_] <= 57) || (json_[pos_] >= 65 && json_[pos_] <= 70) ||
                          (json_[pos_] >= 97 && json_[pos_] <= 102)))
                    {
                        pos_ = start;
                        return JSON_INVALID;
                    }
                    pos_++;
                }
                pos_--;
                break;
            default:
                pos_ = start;
                return JSON_INVALID;
            }
        }
    }

    pos_ = start;

    return JSON_INCOMPLETE;
}

int JsonParser::doParse()
{
    int i, rc;
    JsonType type;
    Token *token;

    for (; pos_ < len_ && json_[pos_] != '\0'; pos_++)
    {
        char c = json_[pos_];

        switch (c)
        {
        case '{':
        case '[':

            token = allocToken();

            if (token == NULL)
            {
                return JSON_ERROR;
            }

            token->type = (c == '{' ? JsonType::OBJECT : JsonType::ARRAY);
            token->start = pos_;

            // belongs to an object
            if (!fatherIdx_.empty())
            {
                Token *tok = &tokens_[fatherIdx_.top()];
                tok->size++;
            }

            // push current token. Since we need to connect attributes of this object to this token
            fatherIdx_.push(tokens_.size() - 1);

            break;

        case '}':
        case ']':

            type = (c == '}' ? JsonType::OBJECT : JsonType::ARRAY);

            if (fatherIdx_.empty())
            {
                return JSON_INVALID;
            }

            /**
             * {
             *      "a": 1,
             *      "b": 2
             * }
             */
            // need to pop the token "b" when we meet '}', since we only pop them when we meet ','
            // so that we can obtain the right superior token later
            if (tokens_[fatherIdx_.top()].type == JsonType::STRING)
            {
                fatherIdx_.pop();
                if (fatherIdx_.empty())
                {
                    return JSON_INVALID;
                }
            }

            // get the superior token of current object
            token = &tokens_[fatherIdx_.top()];
            fatherIdx_.pop();

            if (token->type != type)
            {
                return JSON_INVALID;
            }

            token->end = pos_ + 1;

            break;

        case '\"':

            rc = parseString();
            if (rc < 0)
            {
                return rc;
            }

            if (!fatherIdx_.empty())
            {
                Token *tok = &tokens_[fatherIdx_.top()];
                tok->size++;
            }

            break;

        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;

        case ':':

            fatherIdx_.push(tokens_.size() - 1);
            break;

        case ',':

            // fatherToken might be: ARRAY, STRING when meets ','
            if (!fatherIdx_.empty() && tokens_[fatherIdx_.top()].type == JsonType::STRING)
            {
                // pop when father is a STRING like "a"(check below)
                /**
                 * {
                 *      "a": 0,
                 *      "b": 1
                 * }
                 */
                fatherIdx_.pop();
            }
            break;

        default:

            rc = parsePrimitive();
            if (rc < 0)
            {
                return rc;
            }

            if (!fatherIdx_.empty())
            {
                Token *tok = &tokens_[fatherIdx_.top()];
                tok->size++;
            }

            break;
        }
    }

    // check whether all tokens have been fully parsed
    for (i = tokens_.size() - 1; i >= 0; i--)
    {
        if (tokens_[i].start != -1 && tokens_[i].end == -1)
        {
            return JSON_INCOMPLETE;
        }
    }

    return JSON_OK;
}

JsonResult JsonParser::parse()
{
    int rc = doParse();
    if (rc < 0)
    {
        throw std::runtime_error("parse failed");
    }

    return JsonResult(json_, len_, &tokens_);
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
        if (i + 1 < size)
        {
            idx = skipToken(now_);
            now_ = &tokens_->at(idx);
        }
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

        if (i + 1 < size)
        {
            idx = skipToken(now_);
            now_ = &tokens_->at(idx);
        }
    }

    return ans;
}

template <typename T> T JsonResult::value(T defaultValue)
{
    try
    {
        T val = value<T>();
        return val;
    }
    catch (const std::exception &e)
    {
    }
    return defaultValue;
}

template <typename T> T getValue(JsonResult &json, const char *key, T defaultValue)
{
    try
    {
        T val = json[key].value(defaultValue);
        return val;
    }
    catch (const std::exception &e)
    {
    }
    return defaultValue;
}

template <typename T> T getValue(JsonResult &&json, const char *key, T defaultValue)
{
    try
    {
        T val = json[key].value(defaultValue);
        return val;
    }
    catch (const std::exception &e)
    {
    }
    return defaultValue;
}

#endif
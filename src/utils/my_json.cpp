#include "my_json.h"

#include <stdlib.h>

std::string Token::toString(const char *json)
{
    return std::string(json + start, json + end);
}

JsonParser::JsonParser(const char *json, int len, int maxToken)
    : json_(json), len_(len), tokens_(maxToken), maxSize_(maxToken)
{
    parser_.pos = 0;
    parser_.nextIdx = 0;
    parser_.fatherTokenIdx = -1;
}

JsonParser::~JsonParser()
{
}

Token *JsonParser::allocToken()
{
    Token *tok;
    if (parser_.nextIdx >= maxSize_)
    {
        return NULL;
    }
    tok = &tokens_[parser_.nextIdx];
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
                Token *tok = &tokens_[parser_.fatherTokenIdx];
                tok->size++;
            }

            token->type = (c == '{' ? JsonType::OBJECT : JsonType::ARRAY);
            token->start = parser_.pos;
            parser_.fatherTokenIdx = parser_.nextIdx - 1;

            break;

        case '}':
        case ']':

            type = (c == '}' ? JsonType::OBJECT : JsonType::ARRAY);

            for (i = parser_.nextIdx - 1; i >= 0; i--)
            {
                token = &tokens_[i];
                if (token->start != -1 && token->end == -1)
                {
                    if (token->type != type)
                    {
                        return INVALID;
                    }
                    parser_.fatherTokenIdx = -1;
                    token->end = parser_.pos + 1;
                    break;
                }
            }

            if (i == -1)
            {
                return INVALID;
            }

            for (; i >= 0; i--)
            {
                token = &tokens_[i];
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
                tokens_[parser_.fatherTokenIdx].size++;
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

            if (parser_.fatherTokenIdx != -1 && tokens_[parser_.fatherTokenIdx].type != JsonType::ARRAY &&
                tokens_[parser_.fatherTokenIdx].type != JsonType::OBJECT)
            {
                for (i = parser_.nextIdx - 1; i >= 0; i--)
                {
                    if (tokens_[i].type == JsonType::ARRAY || tokens_[i].type == JsonType::OBJECT)
                    {
                        if (tokens_[i].start != -1 && tokens_[i].end == -1)
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
                tokens_[parser_.fatherTokenIdx].size++;
            }

            break;
        }
    }

    for (i = parser_.nextIdx - 1; i >= 0; i--)
    {
        if (tokens_[i].start != -1 && tokens_[i].end == -1)
        {
            return PART;
        }
    }

    return cnt;
}

Token *JsonParser::get(const char *key, Token *root)
{
    if (root->type != JsonType::OBJECT)
    {
        return NULL;
    }

    int size = root->size;
    int idx = root->idx + 1;

    for (int i = 0; i < size; i++)
    {
        if (tokens_[idx].toString(json_) == key)
        {
            return &tokens_[idx + 1];
        }

        // skip this object
        idx = skipToken(&tokens_[idx + 1]);
    }

    return NULL;
}

Token *JsonParser::get(int arrIdx, Token *root)
{
    if (root->type != JsonType::ARRAY || arrIdx >= root->size)
    {
        return NULL;
    }

    int idx = root->idx + 1;

    for (int i = 0; i < arrIdx; i++)
    {
        // skip this object
        idx = skipToken(&tokens_[idx]);
    }

    return &tokens_[idx];
}

int JsonParser::skipToken(Token *token)
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
            idx = skipToken(&tokens_[idx]) + 1;
        }
        idx--;
        break;
    case JsonType::ARRAY:
        // points to the first inner value
        idx++;
        for (int i = 0; i < size; i++)
        {
            idx = skipToken(&tokens_[idx]);
        }
        break;
    default:
        idx++;
        break;
    }

    return idx;
}
#include "json.h"

#include <stdlib.h>

JsonParser::JsonParser(size_t maxToken)
    : size_(maxToken), tokens_((Token *)calloc(maxToken, 1)), idx_(0) parser_.pos = 0;
parser_.nextToken = 0;
parser_.preToken = -1;
}

JsonParser::~JsonParser()
{
    free(tokens_);
}

Token *JsonParser::allocToken()
{
    Token *tok;
    if (parser_.nextToken >= size_)
    {
        return NULL;
    }
    tok = &tokens_[parser_.nextToken++];
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

int JsonParser::parsePrimitive(const char *js, const size_t len)
{
    Token *token;
    int start = parser_.pos;
    for (; parser_.pos < len && js[parser_.pos] != '\0'; parser_.pos++)
    {
        switch (js[parser_.pos])
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

        if (js[parser_.pos] < 32 || js[parser_.pos] >= 127)
        {
            parser_.pos = start;
            return INVALID;
        }
    }

found:
    if (tokens_ == NULL)
    {
        parser_.pos--;
        return OK;
    }

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

int JsonParser::parseString(const char *js, const size_t len)
{
    Token *token;

    int start = parser_.pos;

    parser_.pos++;

    for (; parser_.pos < len && js[parser_.pos] != '\0'; parser_.pos++)
    {
        char c = js[parser_.pos];

        if (c == '\"')
        {
            if (tokens_ == NULL)
            {
                return OK;
            }

            token = allocToken();

            if (token == NULL)
            {
                parser_.pos = start;
                return ERROR;
            }

            setToken(token, JsonType::STRING, start + 1, parser_.pos);

            return OK;
        }

        if (c == '\\' && parser_.pos + 1 < len)
        {
            parser_.pos++;
            switch (js[parser_.pos])
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
                for (int i = 0; i < 4 && parser_.pos < len && js[parser_.pos] != '\0'; i++)
                {
                    if (!((js[parser_.pos] >= 48 && js[parser_.pos] <= 57) ||
                          (js[parser_.pos] >= 65 && js[parser_.pos] <= 70) ||
                          (js[parser_.pos] >= 97 && js[parser_.pos] <= 102)))
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

    return AGAIN;
}

int JsonParser::doParse(const char *js, const size_t len)
{
    int i, rc;
    JsonType type;
    Token *token;
    int cnt = parser_.nextToken;

    for (; parser_.pos < len && js[parser_.pos] != '\0'; parser_.pos++)
    {
        char c = js[parser_.pos];

        switch (c)
        {
        case '{':
        case '[':

            cnt++;

            if (tokens_ == NULL)
            {
                break;
            }

            token = allocToken();

            if (token == NULL)
            {
                return ERROR;
            }

            if (parser_.preToken != -1)
            {
                Token *tok = &tokens_[parser_.preToken];
                tok->size++;
            }

            token->type = (c == '{' ? JsonType::OBJECT : JsonType::ARRAY);
            token->start = parser_.pos;
            parser_.preToken = parser_.nextToken - 1;

            break;

        case '}':
        case ']':

            if (tokens_ == NULL)
            {
                break;
            }

            type = (c == '}' ? JsonType::OBJECT : JsonType::ARRAY);

            for (i = parser_.nextToken - 1; i >= 0; i--)
            {
                token = &tokens_[i];
                if (token->start != -1 && token->end == -1)
                {
                    if (token->type != type)
                    {
                        return INVALID;
                    }
                    parser_.preToken = -1;
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
                    parser_.preToken = i;
                    break;
                }
            }

            break;

        case '\"':

            rc = parseString(js, len);
            if (rc < 0)
            {
                return rc;
            }

            cnt++;

            if (parser_.preToken != -1 && tokens_ != NULL)
            {
                tokens_[parser_.preToken].size++;
            }

            break;

        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            parser_.preToken = parser_.nextToken - 1;
            break;

        case ',':

            if (tokens_ != NULL && parser_.preToken != -1 && tokens_[parser_.preToken].type != JsonType::ARRAY &&
                tokens_[parser_.preToken].type != JsonType::OBJECT)
            {
                for (i = parser_.nextToken - 1; i >= 0; i--)
                {
                    if (tokens_[i].type == JsonType::ARRAY || tokens_[i].type == JsonType::OBJECT)
                    {
                        if (tokens_[i].start != -1 && tokens_[i].end == -1)
                        {
                            parser_.preToken = i;
                            break;
                        }
                    }
                }
            }
            break;

        default:

            rc = parsePrimitive(js, len);
            if (rc < 0)
            {
                return rc;
            }

            cnt++;

            if (parser_.preToken != -1 && tokens_ != NULL)
            {
                tokens_[parser_.preToken].size++;
            }

            break;
        }
    }

    if (tokens_ != NULL)
    {
        for (i = parser_.nextToken - 1; i >= 0; i--)
        {
            if (tokens_[i].start != -1 && tokens_[i].end == -1)
            {
                return AGAIN;
            }
        }
    }

    return cnt;
}

void JsonParser::parseToken()
{
    
}
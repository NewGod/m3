#include "Config.hh"
#include "Reflect.hh"
#include "Represent.h"

namespace m3
{

ConfigField ConfigField::EOL = 
{
    .name = 0,
    .type = ConfigField::Type::STRING,
    .isCritical = 0,
    .isValid = 0
};

ConfigField ConfigField::CLASS = 
{
    .name = "fieldClassName",
    .type = ConfigField::Type::STRING,
    .isCritical = 1,
    .isValid = 1
};

ConfigObject::~ConfigObject()
{
    if (root)
    {
        cJSON_Delete(root);
    }
    while (!value.empty())
    {
        auto ite = value.begin();
        if (ite->second->type == ConfigField::Type::OBJECT)
        {
            delete (ConfigObject*)(ite->second->object);
        }
        else if (ite->second->type == ConfigField::Type::ARRAY)
        {
            delete (ConfigArray*)(ite->second->array);
        }
        value.erase(ite);
    }
}

ConfigArray::~ConfigArray()
{
    if (arr)
    {
        for (int i = 0; i < len; ++i)
        {
            if (arr[i].type == ConfigField::Type::OBJECT)
            {
                delete (ConfigObject*)arr[i].object;
            }
            else if (arr[i].type == ConfigField::Type::ARRAY)
            {
                delete (ConfigArray*)arr[i].array;
            }
        }
    }
}

#define initNumber(ptr, json)\
    (ptr)->number = (json)->valuedouble

#define initBool(ptr, json)\
    (ptr)->boolean = (json)->type >> 1

#define initString(ptr, json)\
    (ptr)->string = (json)->valuestring

#define initObject(ptr, json, place, onFail)\
    do\
    {\
        int __ret;\
        const char *__className;\
        tryGetFieldClassName(__className, json, place, onFail);\
        iferr (((ptr)->object = (ConfigObject*)Reflect::create(\
            __className)) == 0)\
        {\
            logAllocError(place);\
            return onFail;\
        }\
        iferr ((__ret = (ptr)->object->init(0, json, false)) != 0)\
        {\
            logStackTrace(place, __ret);\
            return onFail;\
        }\
    }\
    while (0)

#define initArray(ptr, json, place, onFail)\
    do\
    {\
        int __ret;\
        iferr (((ptr)->array = snew ConfigArray()) == 0)\
        {\
            logAllocError(place);\
            return onFail;\
        }\
        iferr ((__ret = (ptr)->array->init(json)) != 0)\
        {\
            logStackTrace(place, __ret);\
            return onFail;\
        }\
    }\
    while (0)

int ConfigArray::init(cJSON *root)
{
    int i = 0;
    len = 0;
    for (cJSON *ite = root->child; ite != 0; ite = ite->next)
    {
        ++len;
    }
    iferr ((arr = snew ConfigField[len]) == 0)
    {
        logAllocError("ConfigArray::init");
        return 1;
    }
    for (cJSON *ite = root->child; ite != 0; ite = ite->next, ++i)
    {
        arr[i].name = 0;
        arr[i].isValid = 1;

        if (cJSON_IsNumber(ite))
        {
            arr[i].type = ConfigField::Type::NUMBER;
            initNumber(arr + i, ite);
        }
        else if (cJSON_IsBool(ite))
        {
            arr[i].type = ConfigField::Type::BOOL;
            initBool(arr + i, ite);
        }
        else if (cJSON_IsArray(ite))
        {
            arr[i].type = ConfigField::Type::ARRAY;
            initArray(arr + i, ite, "ConfigArray::init", 2);
        }
        else if (cJSON_IsString(ite))
        {
            arr[i].type = ConfigField::Type::STRING;
            initString(arr + i, ite);
        }
        else if (cJSON_IsObject(ite))
        {
            arr[i].type = ConfigField::Type::OBJECT;
            initObject(arr + i, ite, "ConfigArray::init", 2);
        }
        else
        {
            logError("ConfigArray::init: Invalid data type at #%d.", i);
            return 2;
        }
    }

    return 0;
}

int ConfigObject::init(const char *config, cJSON *root, bool isRoot)
{
    ConfigField *fieldnames = getFields();
    ConfigField *fields;
    int i = 1;
    cJSON *field = 0;
    int len;

    if (isRoot)
    {
        int ret;
        iferr ((ret = readConfig(config)) != 0)
        {
            logStackTrace("ConfigObject::init", ret);
            return 2;
        }
        root = this->root;
    }

    for (len = 0; fieldnames[len].isValid; ++len);
    
    iferr ((fields = snew ConfigField[len + 1]) == 0)
    {
        logAllocError("ConfigObject::init");
        return 1;
    }
    memcpy(fields, fieldnames, sizeof(ConfigField) * (len + 1));

    for (ConfigField *ite = fields; ite->isValid; ++ite, ++i)
    {
        switch (ite->type)
        {
        case ConfigField::Type::NUMBER:
            tryGetConfig(field, root, ite->name, ite->isCritical,
                Number, "ConfigObject::init", i);
            if (field)
            {
                initNumber(ite, field);
            }
            break;
        case ConfigField::Type::BOOL:
            tryGetConfig(field, root, ite->name, ite->isCritical,
                Bool, "ConfigObject::init", i);
            if (field)
            {
                initBool(ite, field);
            }
            break;
        case ConfigField::Type::STRING:
            tryGetConfig(field, root, ite->name, ite->isCritical,
                String, "ConfigObject::init", i);
            if (field)
            {
                initString(ite, field);
            }
            break;
        case ConfigField::Type::OBJECT:
            tryGetConfig(field, root, ite->name, ite->isCritical,
                Object, "ConfigObject::init", i);
            if (field)
            {
                initObject(ite, field, "ConfigObject::init", i);
            }
            break;
        case ConfigField::Type::ARRAY:
            tryGetConfig(field, root, ite->name, ite->isCritical,
                Array, "ConfigObject::init", i);
            if (field)
            {
                initArray(ite, field, "ConfigObject::init", i);
            }
            break;
        default:
            logError("ConfigObject::init: Unrecognized type %d for field %s",
                ite->type, field->string);
            return 3;
        }
        value.insert({ ite->name, ite });
    }

    this->root = isRoot ? root : 0;
    return 0;
}

int ConfigObject::readConfig(const char *config)
{
    FILE *configFile;
    char *fileBuf;
    char *pos;
    int maxLen = MAX_CONFIG_FILE;
    cJSON *obj;
    
    if (config[0] == '-' && config[1] == 0)
    {
        configFile = stdin;
        logMessage("ConfigObject::readConfig: Reading from stdin...");
    }
    else iferr ((configFile = fopen(config, "r")) == 0)
    {
        logError("ConfigObject::readConfig: Cannot open config file.");
        return 1;
    }

    iferr ((pos = fileBuf = snew char[MAX_CONFIG_FILE]) == 0)
    {
        logAllocError("ConfigObject::readConfig");
        return 2;
    }

    while (fgets(pos, maxLen, configFile) != 0)
    {
        int len = strlen(pos);
        pos += len;
        maxLen -= len;

        if (maxLen == 0)
        {
            break;
        }
    }

    iferr ((obj = cJSON_Parse(fileBuf)) == 0)
    {
        logError("ConfigObject::readConfig: cJSON_parse failed(%s)",
            cJSON_GetErrorPtr());
        delete fileBuf;
        return 3;
    }

    root = obj;
    delete fileBuf;

    return 0;
}

ConfigField* ConfigObject::forName(const char *name, ConfigField::Type type)
{
    auto ite = value.find(name);
    if (ite == value.end())
    {
        return 0;
    }
    else if (ite->second->type == type)
    {
        return ite->second;
    }
    else
    {
        return 0;
    }
}

}

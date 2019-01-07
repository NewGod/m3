#ifndef __CONFIG_HH__
#define __CONFIG_HH__

#include <cjson/cJSON.h>

#include "Log.h"
#include "Represent.h"
#include "StringUtils.h"

#define gotConfig(field, obj, name, type)\
    cJSON_Is##type(field = cJSON_GetObjectItemCaseSensitive(obj, name))

#define logCriticalFieldError(place, name, type)\
    logError("%s: Critical field %s(Type %s) not found or type mismatch."\
        , place, name, #type)
    
#define logOptionalFieldMismatch(place, name, type)\
    logError("%s: Optional field %s(Type %s) type mismatch."\
        , place, name, #type)

#define getConfigField(name, obj, type)\
    obj->get##type(#name)

#define tryGetConfig(field, obj, name, critical, type, place, onFail)\
    do\
    {\
        iferr (!gotConfig(field, obj, name, type))\
        {\
            if (critical)\
            {\
                logCriticalFieldError(place, name, type);\
                return onFail;\
            }\
            else if (field != 0)\
            {\
                logOptionalFieldMismatch(place, name, type);\
                return onFail;\
            }\
        }\
    }\
    while (0)

#define tryGetFieldClassName(className, obj, place, onFail)\
    do\
    {\
        cJSON *__tryField;\
        tryGetConfig(__tryField, obj, "fieldClassName", true, \
            String, place, onFail);\
        className = __tryField->valuestring;\
    }\
    while (0)

namespace m3
{

class ConfigObject;
class ConfigArray;

struct ConfigField
{
    static ConfigField EOL;
    static ConfigField CLASS;
    enum Type
    {
        NUMBER, BOOL, STRING, OBJECT, ARRAY
    };
    const char* name;
    Type type;
    bool isCritical;
    bool isValid;
    union
    {
        ConfigObject *object;
        ConfigArray *array;
        const char* string;
        double number;
        bool boolean;
    };
};

struct ConfigArray
{
public:
    ConfigArray(): arr(0) {}
    ~ConfigArray();
    ConfigField* arr;
    int len;
    int init(cJSON *root);
};

// *: Don't copy!
// **: Delete root of `ConfigObject` tree only.
class ConfigObject
{
protected:
    virtual ConfigField* getFields() = 0;
    ConfigField* forName(const char *name, ConfigField::Type type);
    StringKeyHashMap<ConfigField*> value;
    cJSON *root;
public:
    static const int MAX_CONFIG_FILE = 65536;
    ConfigObject():  value(), root(0) {}
    virtual ~ConfigObject();

    inline const char* getString(const char *name)
    {
        ConfigField *field;
        ifsucc ((field = forName(name, ConfigField::Type::STRING)) != 0)
        {
            return field->string;
        }
        return 0;
    }
    inline const char *getFieldClassName()
    {
        return getString("fieldClassName");
    }
    inline int getInt(const char *name)
    {
        ConfigField *field;
        ifsucc ((field = forName(name, ConfigField::Type::NUMBER)) != 0)
        {
            return (int)field->number;
        }
        return 0;
    }
    inline double getDouble(const char *name)
    {
        ConfigField *field;
        ifsucc ((field = forName(name, ConfigField::Type::NUMBER)) != 0)
        {
            return field->number;
        }
        return 0;
    }
    inline bool getBool(const char *name)
    {
        ConfigField *field;
        ifsucc ((field = forName(name, ConfigField::Type::BOOL)) != 0)
        {
            return field->boolean;
        }
        return 0;
    }
    inline ConfigObject* getObject(const char *name)
    {
        ConfigField *field;
        ifsucc ((field = forName(name, ConfigField::Type::OBJECT)) != 0)
        {
            return field->object;
        }
        return 0;
    }
    inline ConfigArray* getArray(const char *name)
    {
        ConfigField *field;
        ifsucc ((field = forName(name, ConfigField::Type::ARRAY)) != 0)
        {
            return field->array;
        }
        return 0;
    }
    inline bool isClass(const char *name)
    {
        return !strcmp(name, getFieldClassName());
    }

    int init(const char *config, cJSON *root = 0, bool isRoot = true);
    int readConfig(const char *config);
};

}

#endif

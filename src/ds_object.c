#include "ds_object.h"


int DSObjectInit(DSObject* obj)
{
    return 0;
}

int DSObjectExit(DSObject* obj)
{
    return 0;
}


int DSEventObjectInit(DSEventObject* obj)
{
    return DSObjectInit((DSObject*)obj);
}

int DSEventObjectExit(DSEventObject* obj)
{
    return DSObjectExit((DSObject*)obj);
}

void DSEventObjectSetCb(DSEventObject* obj, void* cb, void* userData)
{
    obj->userData = userData;
    obj->cb = cb;
}

void* DSEventObjectGetCb(DSEventObject* obj)
{
    return obj->cb;
}

void* DSEventObjectGetUserData(DSEventObject* obj)
{
    return obj->userData;
}

#include <napi.h>
#include "cdio.h"

class ContecDio : public Napi::ObjectWrap<ContecDio>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    ContecDio(const Napi::CallbackInfo &info);

private:
    short id;
    bool inited;
    char buf[256];
    static Napi::FunctionReference s_constructor;
    Napi::Value Init(const Napi::CallbackInfo &info);
    Napi::Value Exit(const Napi::CallbackInfo &info);
    Napi::Value getPort(const Napi::CallbackInfo &info);
    Napi::Value setPort(const Napi::CallbackInfo &info);
};
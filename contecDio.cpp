
#include "contecDio.h"

Napi::FunctionReference ContecDio::s_constructor;

Napi::Object ContecDio::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(env, "contecdio", {InstanceMethod("Init", &ContecDio::Init), InstanceMethod("Exit", &ContecDio::Exit), InstanceMethod("setPort", &ContecDio::setPort), InstanceMethod("getPort", &ContecDio::getPort)});
    s_constructor = Napi::Persistent(func);
    s_constructor.SuppressDestruct();
    exports.Set("contecdio", func);
    return exports;
}

ContecDio::ContecDio(const Napi::CallbackInfo &info) : Napi::ObjectWrap<ContecDio>(info)
{
    inited = false;
}

Napi::Value ContecDio::Init(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (inited)
    {
        return Napi::Number::New(env, -1);
    }
    std::string devId = info[0].As<Napi::String>().Utf8Value();
    long lret = DioInit((char *)devId.c_str(), &id);
    if (lret != DIO_ERR_SUCCESS)
    {
        DioGetErrorString(lret, buf);
        Napi::Error::New(env, "can't open Contec DIO ").ThrowAsJavaScriptException();
    }
    inited = true;
    return Napi::Number::New(env, 0);
}

Napi::Value ContecDio::Exit(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (!inited)
    {
        return Napi::Number::New(env, -1);
    }
    long lret = DioExit(id);
    if (lret != DIO_ERR_SUCCESS)
    {
        DioGetErrorString(lret, buf);
        return Napi::Number::New(env, -2);
    }
    inited = false;
    return Napi::Number::New(env, 0);
}

Napi::Value ContecDio::setPort(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 2)
    {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    short portNo = (short)info[0].As<Napi::Number>().Int32Value();
    if (!info[1].IsBoolean())
    {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    bool value = info[1].As<Napi::Boolean>().Value();
    char data = value ? 1 : 0;
    if (!inited)
    {
        return Napi::Number::New(env, -1);
    }
    long lret = DioOutBit(id, portNo, data);
    if (lret != DIO_ERR_SUCCESS)
    {
        DioGetErrorString(lret, buf);
        return Napi::Number::New(env, -2);
    }
    return Napi::Number::New(env, 0);
}

Napi::Value ContecDio::getPort(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() != 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    short portNo = (short)info[0].As<Napi::Number>().Int32Value();
    unsigned char data = 0;
    long lret = DioInpBit(id, portNo, &data);
    bool result = lret == DIO_ERR_SUCCESS
                      ? (data == 1 ? true : false)
                      : false;
    return Napi::Boolean::New(env, result);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    ContecDio::Init(env, exports);
    return exports;
}

NODE_API_MODULE(contecgpio, Init);
#include <napi.h>
#include "embed.hpp"
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "embed"), Napi::Function::New(env, embed));
    exports.Set(Napi::String::New(env, "unEmbed"), Napi::Function::New(env, unEmbed));
    exports.Set(Napi::String::New(env, "refresh"), Napi::Function::New(env, refresh));
    return exports;
}
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
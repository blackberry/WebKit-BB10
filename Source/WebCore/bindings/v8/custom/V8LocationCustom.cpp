/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "V8Location.h"

#include "BindingState.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "KURL.h"
#include "Location.h"
#include "V8Binding.h"
#include "V8DOMWindow.h"
#include "V8EventListener.h"
#include "V8Utilities.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

void V8Location::hashAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String hash = toWebCoreString(value);

    impl->setHash(hash, activeDOMWindow(state), firstDOMWindow(state));
}

void V8Location::hostAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String host = toWebCoreString(value);

    impl->setHost(host, activeDOMWindow(state), firstDOMWindow(state));
}

void V8Location::hostnameAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String hostname = toWebCoreString(value);

    impl->setHostname(hostname, activeDOMWindow(state), firstDOMWindow(state));
}

void V8Location::hrefAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String href = toWebCoreString(value);

    impl->setHref(href, activeDOMWindow(state), firstDOMWindow(state));
}

void V8Location::pathnameAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String pathname = toWebCoreString(value);

    impl->setPathname(pathname, activeDOMWindow(state), firstDOMWindow(state));
}

void V8Location::portAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String port = toWebCoreString(value);

    impl->setPort(port, activeDOMWindow(state), firstDOMWindow(state));
}

void V8Location::protocolAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String protocol = toWebCoreString(value);

    ExceptionCode ec = 0;
    impl->setProtocol(protocol, activeDOMWindow(state), firstDOMWindow(state), ec);
    if (UNLIKELY(ec))
        setDOMException(ec, info.GetIsolate());
}

void V8Location::searchAttrSetterCustom(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    Location* impl = V8Location::toNative(info.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String search = toWebCoreString(value);

    impl->setSearch(search, activeDOMWindow(state), firstDOMWindow(state));
}

v8::Handle<v8::Value> V8Location::reloadAttrGetterCustom(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    // This is only for getting a unique pointer which we can pass to privateTemplate.
    static const char* privateTemplateUniqueKey = "reloadPrivateTemplate";
    WrapperWorldType currentWorldType = worldType(isolate);
    V8PerIsolateData* data = V8PerIsolateData::from(info.GetIsolate());
    v8::Persistent<v8::FunctionTemplate> privateTemplate = data->privateTemplate(currentWorldType, &privateTemplateUniqueKey, V8Location::reloadMethodCustom, v8Undefined(), v8::Signature::New(data->rawTemplate(&V8Location::info)));

    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8Location::GetTemplate(isolate, currentWorldType));
    if (holder.IsEmpty()) {
        // can only reach here by 'object.__proto__.func', and it should passed
        // domain security check already
        return privateTemplate->GetFunction();
    }
    Location* imp = V8Location::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(BindingState::instance(), imp->frame(), DoNotReportSecurityError)) {
        static const char* sharedTemplateUniqueKey = "reloadSharedTemplate";
        v8::Persistent<v8::FunctionTemplate> sharedTemplate = data->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, V8Location::reloadMethodCustom, v8Undefined(), v8::Signature::New(data->rawTemplate(&V8Location::info)));
        return sharedTemplate->GetFunction();
    }
    return privateTemplate->GetFunction();
}

v8::Handle<v8::Value> V8Location::replaceAttrGetterCustom(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    // This is only for getting a unique pointer which we can pass to privateTemplateMap.
    static const char* privateTemplateUniqueKey = "replacePrivateTemplate";
    WrapperWorldType currentWorldType = worldType(info.GetIsolate());
    V8PerIsolateData* data = V8PerIsolateData::from(info.GetIsolate());
    v8::Persistent<v8::FunctionTemplate> privateTemplate = V8PerIsolateData::from(isolate)->privateTemplate(currentWorldType, &privateTemplateUniqueKey, V8Location::replaceMethodCustom, v8Undefined(), v8::Signature::New(data->rawTemplate(&V8Location::info)));

    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8Location::GetTemplate(isolate, currentWorldType));
    if (holder.IsEmpty()) {
        // can only reach here by 'object.__proto__.func', and it should passed
        // domain security check already
        return privateTemplate->GetFunction();
    }
    Location* imp = V8Location::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(BindingState::instance(), imp->frame(), DoNotReportSecurityError)) {
        static const char* sharedTemplateUniqueKey = "replaceSharedTemplate";
        v8::Persistent<v8::FunctionTemplate> sharedTemplate = V8PerIsolateData::from(info.GetIsolate())->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, V8Location::replaceMethodCustom, v8Undefined(), v8::Signature::New(data->rawTemplate(&V8Location::info)));
        return sharedTemplate->GetFunction();
    }
    return privateTemplate->GetFunction();
}

v8::Handle<v8::Value> V8Location::assignAttrGetterCustom(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    // This is only for getting a unique pointer which we can pass to privateTemplateMap.
    static const char* privateTemplateUniqueKey = "assignPrivateTemplate";
    WrapperWorldType currentWorldType = worldType(info.GetIsolate());
    V8PerIsolateData* data = V8PerIsolateData::from(info.GetIsolate());
    v8::Persistent<v8::FunctionTemplate> privateTemplate = data->privateTemplate(currentWorldType, &privateTemplateUniqueKey, V8Location::assignMethodCustom, v8Undefined(), v8::Signature::New(data->rawTemplate(&V8Location::info)));

    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8Location::GetTemplate(isolate, currentWorldType));
    if (holder.IsEmpty()) {
        // can only reach here by 'object.__proto__.func', and it should passed
        // domain security check already
        return privateTemplate->GetFunction();
    }
    Location* imp = V8Location::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(BindingState::instance(), imp->frame(), DoNotReportSecurityError)) {
        static const char* sharedTemplateUniqueKey = "assignSharedTemplate";
        v8::Persistent<v8::FunctionTemplate> sharedTemplate = data->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, V8Location::assignMethodCustom, v8Undefined(), v8::Signature::New(data->rawTemplate(&V8Location::info)));
        return sharedTemplate->GetFunction();
    }
    return privateTemplate->GetFunction();
}

v8::Handle<v8::Value> V8Location::reloadMethodCustom(const v8::Arguments& args)
{
    Location* impl = V8Location::toNative(args.Holder());
    BindingState* state = BindingState::instance();

    impl->reload(activeDOMWindow(state));
    return v8::Undefined();
}

v8::Handle<v8::Value> V8Location::replaceMethodCustom(const v8::Arguments& args)
{
    Location* impl = V8Location::toNative(args.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String urlString = toWebCoreString(args[0]);

    impl->replace(urlString, activeDOMWindow(state), firstDOMWindow(state));
    return v8::Undefined();
}

v8::Handle<v8::Value> V8Location::assignMethodCustom(const v8::Arguments& args)
{
    Location* impl = V8Location::toNative(args.Holder());
    BindingState* state = BindingState::instance();

    // FIXME: Handle exceptions correctly.
    String urlString = toWebCoreString(args[0]);

    impl->assign(urlString, activeDOMWindow(state), firstDOMWindow(state));
    return v8::Undefined();
}

v8::Handle<v8::Value> V8Location::valueOfMethodCustom(const v8::Arguments& args)
{
    // Just return the this object the way the normal valueOf function
    // on the Object prototype would.  The valueOf function is only
    // added to make sure that it cannot be overwritten on location
    // objects, since that would provide a hook to change the string
    // conversion behavior of location objects.
    return args.This();
}

v8::Handle<v8::Value> V8Location::toStringMethodCustom(const v8::Arguments& args)
{
    v8::Handle<v8::Object> holder = args.Holder();
    Location* imp = V8Location::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(BindingState::instance(), imp->frame()))
        return v8::Undefined();
    String result = imp->href();
    return v8String(result, args.GetIsolate());
}

}  // namespace WebCore

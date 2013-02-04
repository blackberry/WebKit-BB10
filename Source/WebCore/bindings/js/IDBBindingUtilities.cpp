/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INDEXED_DATABASE)
#include "IDBBindingUtilities.h"

#include "IDBKey.h"
#include "JSMainThreadExecState.h"

#include <APICast.h>
#include <DateInstance.h>
#include <JSContextRef.h>

namespace WebCore {

class IDBTemporaryJSContext {
public:
    static IDBTemporaryJSContext* instance()
    {
        DEFINE_STATIC_LOCAL(IDBTemporaryJSContext, s_instance, ());

        return &s_instance;
    }

    JSC::ExecState* exec()
    {
        return toJS(m_context);
    }

private:
    IDBTemporaryJSContext()
    {
        m_context = JSGlobalContextCreate(0);
    }

    ~IDBTemporaryJSContext()
    {
        JSGlobalContextRelease(m_context);
    }

    JSGlobalContextRef m_context;
};

PassRefPtr<IDBKey> createIDBKeyFromValue(JSC::ExecState* exec, JSC::JSValue value)
{
    if (value.isNull())
        return IDBKey::createInvalid();
    if (value.isNumber())
        return IDBKey::createNumber(value.toNumber(exec));
    if (value.isString())
        return IDBKey::createString(value.toString(exec)->value(exec));
    if (value.inherits(&JSC::DateInstance::s_info))
        return IDBKey::createDate(valueToDate(exec, value));
    return 0;
}

void createIDBKeysFromSerializedScriptValuesAndKeyPath(const Vector<RefPtr<SerializedScriptValue>, 0>& values, const IDBKeyPath& keyPath, Vector<RefPtr<IDBKey>, 0>& keys)
{
    JSC::ExecState* exec = isMainThread() ? JSMainThreadExecState::currentState() : IDBTemporaryJSContext::instance()->exec();
    if (!exec)
        return;
    JSC::JSGlobalObject* globalObject = exec->dynamicGlobalObject();

    for (Vector<RefPtr<SerializedScriptValue>, 0>::const_iterator it = values.begin(); it != values.end(); it++) {
        JSC::JSValue jsValue = (*it)->deserialize(exec, globalObject);

        // FIXME: might need to support keyPath as StringArray sometime.
        JSC::Identifier identifier(&exec->globalData(), keyPath.string().utf8().data());
        JSC::JSValue keyValue = asObject(jsValue)->get(exec, identifier);

        RefPtr<IDBKey> key = createIDBKeyFromValue(exec, keyValue);
        keys.append(key);
    }
}

PassRefPtr<SerializedScriptValue> injectIDBKeyIntoSerializedScriptValue(PassRefPtr<IDBKey> key, PassRefPtr<SerializedScriptValue> value, const IDBKeyPath& keyPath)
{
    if (!key->isValid())
        return value;

    JSC::ExecState* exec = isMainThread() ? JSMainThreadExecState::currentState() : IDBTemporaryJSContext::instance()->exec();
    if (!exec)
        return 0;
    JSC::JSGlobalObject* globalObject = exec->dynamicGlobalObject();

    JSC::JSValue jsValue = value->deserialize(exec, globalObject);
    JSC::Identifier propertyName(&exec->globalData(), keyPath.string().utf8().data());


    switch (key->type()) {
    case IDBKey::NumberType:
        asObject(jsValue)->putDirect(exec->globalData(), propertyName, JSC::JSValue(key->number()));
        break;

    case IDBKey::StringType:
    case IDBKey::DateType:
    case IDBKey::InvalidType:
    case IDBKey::ArrayType:
    case IDBKey::MinType:
        ASSERT_NOT_REACHED();
    }

    return SerializedScriptValue::create(exec, jsValue);
}

// FIXME: REBASE
PassRefPtr<IDBKey> createIDBKeyFromSerializedValueAndKeyPath(PassRefPtr<SerializedScriptValue> prpValue, const IDBKeyPath& keyPath)
{
    JSC::ExecState* exec = isMainThread() ? JSMainThreadExecState::currentState() : IDBTemporaryJSContext::instance()->exec();
    if (!exec)
        return 0;
    JSC::JSGlobalObject* globalObject = exec->dynamicGlobalObject();

    JSC::JSValue jsValue = prpValue->deserialize(exec, globalObject);

    // FIXME: might need to support keyPath as StringArray sometime.
    JSC::Identifier identifier(&exec->globalData(), keyPath.string().utf8().data());
    JSC::JSValue keyValue = asObject(jsValue)->get(exec, identifier);

    RefPtr<IDBKey> key = createIDBKeyFromValue(exec, keyValue);
    return PassRefPtr<IDBKey>(key);
}

// FIXME: REBASE
PassRefPtr<SerializedScriptValue> injectIDBKeyIntoSerializedValue(PassRefPtr<IDBKey> key, PassRefPtr<SerializedScriptValue> value, const IDBKeyPath& keyPath)
{
    if (!key->isValid())
        return value;

    JSC::ExecState* exec = isMainThread() ? JSMainThreadExecState::currentState() : IDBTemporaryJSContext::instance()->exec();
    if (!exec)
        return 0;
    JSC::JSGlobalObject* globalObject = exec->dynamicGlobalObject();

    JSC::JSValue jsValue = value->deserialize(exec, globalObject);
    JSC::Identifier propertyName(&exec->globalData(), keyPath.string().utf8().data());


    switch (key->type()) {
    case IDBKey::NumberType:
        asObject(jsValue)->putDirect(exec->globalData(), propertyName, JSC::JSValue(key->number()));
        break;

    case IDBKey::StringType:
    case IDBKey::DateType:
    case IDBKey::InvalidType:
    case IDBKey::ArrayType:
    case IDBKey::MinType:
        ASSERT_NOT_REACHED();
    }

    return SerializedScriptValue::create(exec, jsValue);
}
} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

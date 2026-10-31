#pragma once
#include "crt.h"
#include "../plugin_memory.h"
// Upstream defines inline floating conversion helpers even for text/integer
// clients. Declaration permits parsing; using them requires additional numeric
// support and will fail the strict link. This profile does not fake conversion.
extern "C" double strtod(const char*,char**);
// Read C headers before scoped call substitution so their declarations are
// never renamed. Only the pinned implementation's numeric calls are bridged.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <new>
#if defined(SHVNX_PLUGIN_NUMERIC_RUNTIME) && SHVNX_PLUGIN_NUMERIC_RUNTIME
#include "numeric.h"
#define strtod(text,end) ::ScriptHook::Numeric::Detail::XmlNumber(text,end)
#define snprintf(out,cap,fmt,precision,value) ::ScriptHook::Numeric::Detail::XmlPrint(out,cap,fmt,precision,value)
#else
// The integer-only C formatter must not silently accept an XML floating setter.
// This declaration intentionally has no implementation in the C-only profile.
extern "C" int shvXmlFloatingRequiresNumericProfile(char*,size_t,const char*,int,double);
#define snprintf(out,cap,fmt,precision,value) shvXmlFloatingRequiresNumericProfile(out,cap,fmt,precision,value)
#endif
// Supported profile: UTF-8 memory DOM, no STL/streams, exceptions or XPath.
// Include this before upstream pugixml headers; build with c_runtime=true.
#if defined(HEADER_PUGIXML_HPP) || defined(HEADER_PUGICONFIG_HPP)
#error Include shv/xml.h before pugixml headers to select the NX profile
#endif
#define PUGIXML_NO_STL
#define PUGIXML_NO_EXCEPTIONS
#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
// Release assertions and a smaller serializer scratch buffer on 64 KiB stacks.
#define PUGIXML_MEMORY_OUTPUT_STACK 2048
#ifndef NDEBUG
#define SHVNX_XML_RESTORE_ASSERTS
#define NDEBUG
#endif
#include "../../sdk/third_party/pugixml/pugixml.hpp"
#if defined(SHVNX_PLUGIN_NUMERIC_RUNTIME) && SHVNX_PLUGIN_NUMERIC_RUNTIME
#undef strtod
#endif
#undef snprintf
#ifdef SHVNX_XML_RESTORE_ASSERTS
#undef NDEBUG
#undef SHVNX_XML_RESTORE_ASSERTS
#include <assert.h>
#endif
namespace ScriptHook::Xml {
// Memory writer with an explicit complete/overflow result. No partial document
// is presented as a successful serialization. Output is NUL terminated if valid.
class BufferWriter final:public pugi::xml_writer {
    char* out;size_t capacity,used{};bool good;
public:
    BufferWriter(char* p,size_t n):out(p),capacity(n?n-1:0),good(p&&n){if(good)*out=0;}
    void write(const void* data,size_t bytes)override{
        if(!good)return;
        if(bytes>capacity-used){good=false;return;}
        const auto* p=static_cast<const char*>(data);for(size_t i=0;i<bytes;++i)out[used+i]=p[i];used+=bytes;out[used]=0;
    }
    size_t Bytes()const{return used;}
    bool Complete()const{return good;}
    // Required only by the virtual deleting-destructor slot. Stack instances
    // are destroyed normally; heap instances must come from the plugin heap.
    static void operator delete(void* p)noexcept{(void)scriptFree(p);}
};
inline bool Save(const pugi::xml_document& doc,char* out,size_t capacity,size_t& bytes){
    BufferWriter writer(out,capacity);doc.save(writer,"",pugi::format_raw,pugi::encoding_utf8);bytes=writer.Bytes();return writer.Complete();
}
}

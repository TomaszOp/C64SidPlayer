/**
 * @file NvsSettings.cpp
 * @brief Implementation of NvsSettings on top of the Preferences library (visitor-based load/save/print).
 */
#include "NvsSettings.h"
#include <Preferences.h>

// Reading / writing / printing one field - overloads for every supported type.
/**
 * @brief Reads one value from NVS (get() overloads, one per supported type).
 * @param p Open Preferences.
 * @param k NVS key.
 * @param v Value; keeps its current value when the key is missing.
 */
static void get(Preferences& p, const char* k, bool& v)     { v = p.getBool(k, v); }
static void get(Preferences& p, const char* k, uint8_t& v)  { v = p.getUChar(k, v); }
static void get(Preferences& p, const char* k, int8_t& v)   { v = p.getChar(k, v); }
static void get(Preferences& p, const char* k, uint16_t& v) { v = p.getUShort(k, v); }
static void get(Preferences& p, const char* k, int16_t& v)  { v = p.getShort(k, v); }
static void get(Preferences& p, const char* k, uint32_t& v) { v = p.getULong(k, v); }
static void get(Preferences& p, const char* k, int32_t& v)  { v = p.getLong(k, v); }
static void get(Preferences& p, const char* k, float& v)    { v = p.getFloat(k, v); }
static void get(Preferences& p, const char* k, String& v)   { v = p.getString(k, v); }

/**
 * @brief Writes one value to NVS (put() overloads, one per supported type).
 * @param p Open Preferences.
 * @param k NVS key.
 * @param v Value.
 * @return true if written successfully.
 */
static bool put(Preferences& p, const char* k, bool v)          { return p.putBool(k, v); }
static bool put(Preferences& p, const char* k, uint8_t v)       { return p.putUChar(k, v); }
static bool put(Preferences& p, const char* k, int8_t v)        { return p.putChar(k, v); }
static bool put(Preferences& p, const char* k, uint16_t v)      { return p.putUShort(k, v); }
static bool put(Preferences& p, const char* k, int16_t v)       { return p.putShort(k, v); }
static bool put(Preferences& p, const char* k, uint32_t v)      { return p.putULong(k, v); }
static bool put(Preferences& p, const char* k, int32_t v)       { return p.putLong(k, v); }
static bool put(Preferences& p, const char* k, float v)         { return p.putFloat(k, v); }
static bool put(Preferences& p, const char* k, const String& v) { return p.putString(k, v) == v.length(); }

/**
 * @brief Prints one value (printValue() overloads; bool as "tak"/"nie", String in quotes).
 * @param o Output.
 * @param v Value.
 */
static void printValue(Print& o, bool v)          { o.print(v ? "tak" : "nie"); }
static void printValue(Print& o, uint8_t v)       { o.print(v); }
static void printValue(Print& o, int8_t v)        { o.print(int(v)); }
static void printValue(Print& o, uint16_t v)      { o.print(v); }
static void printValue(Print& o, int16_t v)       { o.print(v); }
static void printValue(Print& o, uint32_t v)      { o.print(v); }
static void printValue(Print& o, int32_t v)       { o.print(v); }
static void printValue(Print& o, float v)         { o.print(v, 2); }
static void printValue(Print& o, const String& v) { o.print('"'); o.print(v); o.print('"'); }

/**
 * @brief Checks whether the value in NVS differs from the current one (or is missing).
 * @param p Open Preferences.
 * @param k NVS key.
 * @param v Current value.
 * @return true if different or missing.
 */
template <class T> static bool differs(Preferences& p, const char* k, const T& v) {
    if (!p.isKey(k)) return true;
    T stored = v;
    get(p, k, stored);
    return !(stored == v);
}

/**
 * @brief Turns one operation (a template functor) into a Visitor for all types.
 * @tparam Op Functor with operator()(key, name, value&, default).
 */
template <class Op> struct VisitorFor : NvsSettings::Visitor {
    Op op;   ///< Operation called for every field.
    /**
     * @brief Wraps an operation.
     * @param o Operation (copied).
     */
    explicit VisitorFor(const Op& o) : op(o) {}
#define NVS_VISIT_TYPE(T) void field(const char* k, const char* n, T& v, const T& d) override { op(k, n, v, d); }
    NVS_VISIT_TYPE(bool) NVS_VISIT_TYPE(uint8_t) NVS_VISIT_TYPE(int8_t)
    NVS_VISIT_TYPE(uint16_t) NVS_VISIT_TYPE(int16_t) NVS_VISIT_TYPE(uint32_t)
    NVS_VISIT_TYPE(int32_t) NVS_VISIT_TYPE(float) NVS_VISIT_TYPE(String)
#undef NVS_VISIT_TYPE
};
/**
 * @brief Creates a VisitorFor with the type deduced from the argument.
 * @param op Operation.
 * @return Visitor calling op for every field.
 */
template <class Op> static VisitorFor<Op> visitorFor(const Op& op) { return VisitorFor<Op>(op); }

static const char* VERSION_KEY = "_ver";   ///< NVS key holding the format version.

/** @brief Operation: sets the field to its default value. */
struct OpDefault { template <class T> void operator()(const char*, const char*, T& v, const T& d) { v = d; } };
/** @brief Operation: reads the field from NVS. */
struct OpGet {
    Preferences* p;   ///< Open Preferences.
    template <class T> void operator()(const char* k, const char*, T& v, const T&) { get(*p, k, v); }
};
/** @brief Operation: writes the field to NVS when it differs (or always when all is set). */
struct OpPut {
    Preferences* p; bool* ok; bool all;   ///< Open Preferences; cleared on a write error; write every field.
    template <class T> void operator()(const char* k, const char*, T& v, const T&) {
        if (all || differs(*p, k, v)) *ok = put(*p, k, v) && *ok;
    }
};
/** @brief Operation: sets *changed when the field differs from NVS. */
struct OpChanged {
    Preferences* p; bool* changed;   ///< Open Preferences; result flag.
    template <class T> void operator()(const char* k, const char*, T& v, const T&) {
        if (!*changed && differs(*p, k, v)) *changed = true;
    }
};
/** @brief Operation: prints "  name = value". */
struct OpPrint {
    Print* out;   ///< Output.
    template <class T> void operator()(const char*, const char* n, T& v, const T&) {
        out->print("  "); out->print(n); out->print(" = "); printValue(*out, v); out->println();
    }
};

bool NvsSettings::load() {
    auto def = visitorFor(OpDefault{});
    visit(def);
    Preferences p;
    if (!p.begin(ns_, true)) return false;           // przestrzen jeszcze nie istnieje
    bool ok = p.getUShort(VERSION_KEY, 0) == version_;
    if (ok) {
        auto rd = visitorFor(OpGet{&p});
        visit(rd);
    }
    p.end();
    return ok;
}

bool NvsSettings::save() {
    Preferences p;
    if (!p.begin(ns_, false)) return false;
    bool ok = true, all = false;
    if (p.getUShort(VERSION_KEY, 0) != version_) {
        p.clear();                                   // stary format - zacznij od zera
        ok = p.putUShort(VERSION_KEY, version_) > 0;
        all = true;
    }
    auto wr = visitorFor(OpPut{&p, &ok, all});     // zapis tylko roznic - oszczedza flash
    visit(wr);
    p.end();
    return ok;
}

void NvsSettings::setDefault(bool eraseNvs) {
    auto def = visitorFor(OpDefault{});
    visit(def);
    if (eraseNvs) {
        Preferences p;
        if (p.begin(ns_, false)) { p.clear(); p.end(); }
    }
}

bool NvsSettings::changed() {
    Preferences p;
    if (!p.begin(ns_, true)) return true;            // nic jeszcze nie zapisano
    bool ch = p.getUShort(VERSION_KEY, 0) != version_;
    auto cmp = visitorFor(OpChanged{&p, &ch});
    if (!ch) visit(cmp);
    p.end();
    return ch;
}

void NvsSettings::print(Print& out) {
    auto pr = visitorFor(OpPrint{&out});
    visit(pr);
}

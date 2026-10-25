/**
 * @file NvsSettingsClass.h
 * @brief Settings class generator - see the description in NvsSettings.h.
 *
 * Define NVS_CLASS, NVS_DEF, NVS_NAMESPACE and NVS_VERSION before including it.
 * (no #pragma once - the file is included once for every settings set)
 */
#include "NvsSettings.h"

#if !defined(NVS_CLASS) || !defined(NVS_DEF) || !defined(NVS_NAMESPACE) || !defined(NVS_VERSION)
#error "Zdefiniuj NVS_CLASS, NVS_DEF, NVS_NAMESPACE i NVS_VERSION przed #include <NvsSettingsClass.h>"
#endif

/**
 * @brief Settings class generated from NVS_DEF: one public field per SETTING line.
 */
class NVS_CLASS : public NvsSettings {
public:
    // one field per SETTING(type, name, key, def) line, initialized with the default value
#define SETTING(type, name, key, def) type name = def;
#include NVS_DEF
#undef SETTING

    /**
     * @brief Creates the settings with default values.
     * @param nvsNamespace NVS namespace (default NVS_NAMESPACE).
     */
    explicit NVS_CLASS(const char* nvsNamespace = NVS_NAMESPACE) : NvsSettings(nvsNamespace, NVS_VERSION) {}

protected:
    /**
     * @brief Calls v.field(key, name, value, default) for every SETTING line.
     * @param v Visitor.
     */
    void visit(Visitor& v) override {
#define SETTING(type, name, key, def) { static const type d_ = def; v.field(key, #name, name, d_); }
#include NVS_DEF
#undef SETTING
    }
};

#undef NVS_CLASS
#undef NVS_DEF
#undef NVS_NAMESPACE
#undef NVS_VERSION

#pragma once
#include <cstring>
#include <map>
#include <vector>
#include "component.h"
namespace esphome {
inline std::map<uint32_t, std::vector<uint8_t>> records;
inline bool fail_save = false;
inline unsigned saves = 0;
class ESPPreferenceObject {
 uint32_t key_ = 0;
 public:
 ESPPreferenceObject() = default;
 explicit ESPPreferenceObject(uint32_t key) : key_(key) {}
 template<class T> bool load(T *v) { auto it=records.find(key_); if(it==records.end() || it->second.size()!=sizeof(T)) return false; std::memcpy(v,it->second.data(),sizeof(T)); return true; }
 template<class T> bool save(const T *v) { if(fail_save) return false; auto p=reinterpret_cast<const uint8_t*>(v); records[key_]=std::vector<uint8_t>(p,p+sizeof(T)); ++saves; return true; }
};
class Preferences { public: template<class T> ESPPreferenceObject make_preference(uint32_t key, bool) { return ESPPreferenceObject(key); } };
inline Preferences preferences;
inline Preferences *global_preferences = &preferences;
}

﻿#pragma once

#define NOMINMAX

#include "thprac_version.h"
#include "thprac_gui_components.h"
#include "thprac_gui_impl_dx8.h"
#include "thprac_gui_impl_dx9.h"
#include "thprac_gui_impl_win32.h"
#include "thprac_hook.h"
#include "thprac_locale_def.h"
#include "thprac_log.h"
#include "thprac_data_anly.h"

#include <Windows.h>
#include <cstdint>
#include <imgui.h>
#include <implot.h>
#include <memory>
#pragma warning(push)
#pragma warning(disable : 26451)
#pragma warning(disable : 26495)
#pragma warning(disable : 33010)
#pragma warning(disable : 26819)
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#pragma warning(pop)
#include <random>
#include <string>
#include <tsl/robin_map.h>
#include <utility>
#include <vector>

#define MB_INFO(str) MessageBoxA(NULL, str, str, MB_OK);

namespace THPrac {

#pragma region Locale
std::string utf16_to_utf8(const std::wstring& utf16);
std::string utf16_to_utf8(const wchar_t* utf16);
std::wstring utf8_to_utf16(const std::string& utf8);
std::wstring utf8_to_utf16(const char* utf8);
std::string utf16_to_mb(const std::wstring& utf16);
std::string utf16_to_mb(const wchar_t* utf16);
std::wstring mb_to_utf16(const std::string& utf8);
std::wstring mb_to_utf16(const char* utf8);
#pragma endregion

#pragma region Path
std::string GetSuffixFromPath(const char* pathC);
std::string GetSuffixFromPath(std::string& path);
std::string GetDirFromFullPath(std::string& dir);
std::wstring GetDirFromFullPath(std::wstring& dir);
std::string GetNameFromFullPath(std::string& dir);
std::wstring GetNameFromFullPath(std::wstring& dir);
std::string GetCleanedPath(std::string& path);
std::wstring GetCleanedPath(std::wstring& path);
std::string GetUnifiedPath(std::string& path);
std::wstring GetUnifiedPath(std::wstring& path);
#pragma endregion

#pragma region Gui Wrapper

enum game_gui_impl {
    IMPL_WIN32_DX8,
    IMPL_WIN32_DX9
};

void GameGuiInit(game_gui_impl impl, int device, int hwnd, int wndproc_addr,
    Gui::ingame_input_gen_t input_gen, int reg1, int reg2, int reg3 = 0,
    int wnd_size_flag = -1, float x = 640.0f, float y = 480.0f);
void GameGuiBegin(game_gui_impl impl, bool game_nav = true);
void GameGuiEnd(bool draw_cursor = false);
void GameGuiRender(game_gui_impl impl);
void GameToggleIME(bool toggle);
void TryKeepUpRefreshRate(void* address);
void TryKeepUpRefreshRate(void* address, void* address2);

#pragma endregion

#pragma region Advanced Options Menu

struct RecordedValue {
    enum Type {
        TYPE_INT,
        TYPE_FLOAT,
        TYPE_DOUBLE,
        TYPE_INT64,
    };

    Type type;
    std::string name;
    union {
        int i;
        float f;
        double d;
        int64_t i64;
    } value;
    std::string format;

    RecordedValue(std::string n, int i, const char* fmt = "%d")
    {
        type = TYPE_INT;
        name = n;
        value.i = i;
        format = fmt;
    }
    RecordedValue(std::string n, float f, const char* fmt = "%f")
    {
        type = TYPE_FLOAT;
        name = n;
        value.f = f;
        format = fmt;
    }
    RecordedValue(std::string n, double d, const char* fmt = "%lf")
    {
        type = TYPE_DOUBLE;
        name = n;
        value.d = d;
        format = fmt;
    }
    RecordedValue(std::string n, int64_t i64, const char* fmt = "%ll")
    {
        type = TYPE_INT64;
        name = n;
        value.i64 = i64;
        format = fmt;
    }
};

struct adv_opt_ctx {
    int fps_status = 0;
    int fps = 60;
    double fps_dbl = 1.0 / 60.0;
    int fps_replay_slow = 0;
    int fps_replay_fast = 0;
    int fps_debug_acc = 0;
    uintptr_t vpatch_base = 0;

    bool data_rec_toggle = false;
    std::function<void(std::vector<RecordedValue>&)> data_rec_func;
    std::wstring data_rec_dir;

    bool all_clear_bonus = false;

    typedef bool __stdcall oilp_set_fps_t(int fps);
    oilp_set_fps_t* oilp_set_game_fps = NULL;
    oilp_set_fps_t* oilp_set_replay_skip_fps = NULL;
    oilp_set_fps_t* oilp_set_replay_slow_fps = NULL;
};

void CenteredText(const char* text, float wndX);
float GetRelWidth(float rel);
float GetRelHeight(float rel);
void CalcFileHash(const wchar_t* file_name, uint64_t hash[2]);
void HelpMarker(const char* desc);
template <th_glossary_t name>
static bool BeginOptGroup()
{
    static bool group_status = true;
    ImGui::SetNextItemOpen(group_status);
    group_status = ImGui::CollapsingHeader(XSTR(name), ImGuiTreeNodeFlags_None);
    if (group_status)
        ImGui::Indent();
    return group_status;
}
static void EndOptGroup()
{
    ImGui::Unindent();
}
typedef void __stdcall FPSHelperCallback(int32_t);
void OILPInit(adv_opt_ctx& ctx);
int FPSHelper(adv_opt_ctx& ctx, bool repStatus, bool vpFast, bool vpSlow, FPSHelperCallback* callback);
bool GameFPSOpt(adv_opt_ctx& ctx);
bool GameplayOpt(adv_opt_ctx& ctx);
bool DataRecOpt(adv_opt_ctx& ctx, bool preUpd = false, bool isInGame = false);
void AboutOpt(const char* thanks_text = nullptr);

#pragma endregion

#pragma region Game BGM

template <
    int32_t play_addr,
    int32_t stop_addr,
    int32_t pause_addr,
    int32_t resume_addr,
    int32_t caller_addr>
static bool ElBgmTest(bool hotkey_status, bool practice_status,
    int32_t retn_addr, int32_t bgm_cmd, int32_t bgm_param, int32_t caller)
{
    static bool mElStatus { false };
    static int mLockBgmId { -1 };

    bool hotkey = hotkey_status;
    bool is_practice = practice_status;

    switch (retn_addr) {
    case play_addr:
        if (caller == caller_addr) {
            if (mLockBgmId == -1)
                mLockBgmId = bgm_param;
            if (mLockBgmId != bgm_param) {
                mLockBgmId = -1;
                mElStatus = 0;
            } else if (!mElStatus && hotkey) {
                mElStatus = 1;
                return false;
            }
        }
        if (mLockBgmId >= 0 && mLockBgmId != bgm_param) {
            mLockBgmId = -1;
            mElStatus = 0;
        }
        break;
    case stop_addr:
        if (mLockBgmId >= 0) {
            mLockBgmId = -1;
            // Quitting or disabled
            if (!is_practice || !hotkey)
                mElStatus = 0;
        }
        break;
    case pause_addr:
        if (mLockBgmId >= 0) {
            if (hotkey)
                mElStatus = 1;
            else
                mElStatus = 0;
        }
        break;
    case resume_addr:
        if (mLockBgmId >= 0) {
            if (!mElStatus && hotkey) {
                mElStatus = 1;
                return false;
            }
        }
        break;
    default:
        break;
    }

    return mElStatus;
}

template <
    int32_t play_addr,
    int32_t play_addr_2,
    int32_t stop_addr,
    int32_t pause_addr,
    int32_t resume_addr,
    int32_t caller_addr>
static bool ElBgmTestTemp(bool hotkey_status, bool practice_status,
    int32_t retn_addr, int32_t bgm_cmd, int32_t bgm_param, int32_t caller)
{
    static bool mElStatus { false };
    static int mLockBgmId { -1 };

    bool hotkey = hotkey_status;
    bool is_practice = practice_status;

    switch (retn_addr) {
    case play_addr:
    case play_addr_2:
        if (caller == caller_addr) {
            if (mLockBgmId == -1)
                mLockBgmId = bgm_param;
            if (mLockBgmId != bgm_param) {
                mLockBgmId = -1;
                mElStatus = 0;
            } else if (!mElStatus && hotkey) {
                mElStatus = 1;
                return false;
            }
        }
        if (mLockBgmId >= 0 && mLockBgmId != bgm_param) {
            mLockBgmId = -1;
            mElStatus = 0;
        }
        break;
    case stop_addr:
        if (mLockBgmId >= 0) {
            mLockBgmId = -1;
            // Quitting or disabled
            if (!is_practice || !hotkey)
                mElStatus = 0;
        }
        break;
    case pause_addr:
        if (mLockBgmId >= 0) {
            if (hotkey)
                mElStatus = 1;
            else
                mElStatus = 0;
        }
        break;
    case resume_addr:
        if (mLockBgmId >= 0) {
            if (!mElStatus && hotkey) {
                mElStatus = 1;
                return false;
            }
        }
        break;
    default:
        break;
    }

    return mElStatus;
}

#pragma endregion

#pragma region Replay System

typedef void*(__cdecl* p_malloc)(size_t size);

bool ReplaySaveParam(const wchar_t* rep_path, std::string& param);
bool ReplayLoadParam(const wchar_t* rep_path, std::string& param);

#pragma endregion

#pragma region Json

#define ParseJson()                                \
    Reset();                                       \
    rapidjson::Document param;                     \
    if (param.Parse(json.c_str()).HasParseError()) \
        return false;
#define ParseJsonNoReset()                         \
    rapidjson::Document param;                     \
    if (param.Parse(json.c_str()).HasParseError()) \
        return false;
#define CreateJson()           \
    rapidjson::Document param; \
    param.SetObject();         \
    auto& jalloc = param.GetAllocator();
#define ReturnJson()                                       \
    rapidjson::StringBuffer sb;                            \
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb); \
    param.Accept(writer);                                  \
    return sb.GetString();
#define ForceJsonValue(value_name, comparator)                             \
    if (!param.HasMember(#value_name) || param[#value_name] != comparator) \
        return false;
#define GetJsonValue(value_name)                                       \
    if (param.HasMember(#value_name) && param[#value_name].IsNumber()) \
        value_name = (decltype(value_name))param[#value_name].GetDouble();
#define GetJsonValueEx(value_name, type)                               \
    if (param.HasMember(#value_name) && param[#value_name].Is##type()) \
        value_name = (decltype(value_name))param[#value_name].Get##type();
#define GetJsonValueAlt(value_name, valueVar, type)                    \
    if (param.HasMember(#value_name) && param[#value_name].Is##type()) \
        valueVar = param[#value_name].Get##type();
#define AddJsonValue(value_name)                                           \
    {                                                                      \
        rapidjson::Value __key_##value_name(#value_name, jalloc);          \
        rapidjson::Value __value_##value_name(value_name);                 \
        param.AddMember(__key_##value_name, __value_##value_name, jalloc); \
    }
#define AddJsonValueEx(value_name, ...)                                    \
    {                                                                      \
        rapidjson::Value __key_##value_name(#value_name, jalloc);          \
        rapidjson::Value __value_##value_name(__VA_ARGS__);                \
        param.AddMember(__key_##value_name, __value_##value_name, jalloc); \
    }

#pragma endregion

#pragma region Virtual File System
class VFile {
public:
    VFile() = default;

    void SetFile(void* buffer, size_t size)
    {
        mBuffer = (uint8_t*)buffer;
        mSize = size;
        mPos = 0;
    }
    void SetPos(size_t pos)
    {
        if (pos >= mSize)
            return;
        mPos = pos;
    }
    size_t GetPos()
    {
        return mPos;
    }

    void Write(const char* data);
    void Write(void* data, unsigned int length);
    void Read(void* buffer, unsigned int length);

    template <typename T>
    VFile& operator<<(T data)
    {
        *(T*)(mBuffer + mPos) = data;
        mPos += sizeof(T);
        return *this;
    }
    template <typename T>
    VFile& operator>>(T& data)
    {
        Read(&data, sizeof(T));
        return *this;
    }
    template <typename T>
    VFile& operator<<(std::pair<size_t, T> data)
    {
        SetPos(data.first);
        operator<<(data.second);
        return *this;
    }
    template <typename T>
    VFile& operator<<(std::pair<int, T> data)
    {
        SetPos(data.first);
        operator<<(data.second);
        return *this;
    }
    template <>
    VFile& operator<<<const char*>(const char* data)
    {
        Write(data);
        return *this;
    }

private:
    uint8_t* mBuffer = nullptr;
    size_t mSize = 0;
    size_t mPos = 0;
};

// Typedef
enum VFS_TYPE {
    VFS_TH11,
    VFS_TH08,
    VFS_TH07,
    VFS_TH06,
};
typedef void* (*vfs_listener)(const char* file_name, int32_t* file_size, int unk, void* buffer);

// API
void VFSHook(VFS_TYPE type, void* addr);
void VFSAddListener(const char* file_name, vfs_listener onCall, vfs_listener onLoad);
void* VFSOriginal(const char* file_name, int32_t* file_size, int32_t is_file);

#pragma endregion

#pragma region Memory Helper

template <class... Args>
inline uint32_t GetMemContent(int addr, int offset, Args... rest)
{
    return GetMemContent((int)(*((uint32_t*)addr) + (uint32_t)offset), rest...);
}
inline uint32_t GetMemContent(int addr, int offset)
{
    return *((uint32_t*)(*((uint32_t*)addr) + (uint32_t)offset));
}
inline uint32_t GetMemContent(int addr)
{
    return *((uint32_t*)addr);
}

template <class... Args>
inline uint32_t GetMemAddr(int addr, int offset, Args... rest)
{
    return GetMemAddr((int)(*((uint32_t*)addr) + (uint32_t)offset), rest...);
}
inline uint32_t GetMemAddr(int addr, int offset)
{
    return (*((uint32_t*)addr) + (uint32_t)offset);
}
inline uint32_t GetMemAddr(int addr)
{
    return (uint32_t)addr;
}

#define MDARRAY(arr, idx, size_of_subarray) (arr + idx * size_of_subarray)

#pragma endregion

#pragma region ECL Helper

class ECLHelper : public VFile {
public:
    ECLHelper() = default;

    void SetBaseAddr(void* addr)
    {
        mPtrToBuffer = (uint8_t*)addr;
        VFile::SetFile((uint8_t*)(*(uint32_t*)addr), 0x99999);
    }
    void SetFile(unsigned int ordinal)
    {
        VFile::SetFile((uint8_t*)(*(uint32_t*)(mPtrToBuffer + ordinal * 4)), 0x99999);
    }

private:
    uint8_t* mPtrToBuffer = nullptr;
};

template <typename T>
inline std::pair<size_t, T> ECLX(size_t pos, T data)
{
    return std::make_pair(pos, data);
}

typedef void (*ecl_patch_func)(ECLHelper& ecl);
#define ECLPatch(name, ...) void name(ECLHelper& ecl, __VA_ARGS__)

#pragma endregion

#pragma region Quick Config

typedef void __stdcall QuickCfgHotkeyCallback(std::string&, bool);
int QuickCfg(const char* game, QuickCfgHotkeyCallback* callback);
bool QuickCfgHintText(bool reset = false);

#pragma endregion

#pragma region Rounding

/** round n down to nearest multiple of m */
inline long RoundDown(long n, long m)
{
    return n >= 0 ? (n / m) * m : ((n - m + 1) / m) * m;
}

/** round n up to nearest multiple of m */
inline long RoundUp(long n, long m)
{
    return n >= 0 ? ((n + m - 1) / m) * m : (n / m) * m;
}

#pragma endregion

template <typename T>
static std::function<T(void)> GetRndGenerator(T min, T max, std::mt19937::result_type seed = 0)
{
    // std::mt19937::result_type seed = time(0);
    if (!seed) {
        seed = (std::mt19937::result_type)time(0);
        // std::random_device rd;
        // seed = rd();
    }
    auto dice_rand = std::bind(std::uniform_int_distribution<T>(min, max), std::mt19937(seed));
    return dice_rand;
}
DWORD WINAPI CheckDLLFunction(const wchar_t* path, const char* funcName);
}

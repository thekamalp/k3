// k3 graphics library
// internal header file for win32 platform
#pragma once

#include "k3internal.h"

#include <Windows.h>
#include <dsound.h>
#include <hidsdi.h>

struct k3win32JoyAxisRange
{
    uint64_t min;
    uint64_t max;
};

class k3win32JoyObj;
typedef k3ptr<k3win32JoyObj> k3win32Joy;
class k3win32JoyObj : public k3obj
{
public:
    static k3win32Joy Create(HANDLE hDevice, uint32_t dev_id);

    k3win32JoyObj();
	k3win32JoyObj(HANDLE hDevice);
	virtual ~k3win32JoyObj();

	virtual K3API k3objType getObjType() const
	{
		return k3objType::UNKNOWN;  // not publically visible object
	}

	virtual void Poll();
	virtual void SetAttribute(k3joyAttr attr_type, uint32_t num_values, float* values);
	uint32_t getDevId();
	const k3joyInfo* getJoyInfo();
	const k3joyState* getJoyState();
	bool buttonHasChanged(uint32_t button);
	bool axisHasChanged(uint32_t axis);

	static float NormalizeJoystickPos(ULONG pos, k3win32JoyAxisRange range);
	static k3joyAxis UsageToAxisType(USAGE page, USAGE usage);
protected:
	void InitData();
	k3joyInfo _joy_info;
	k3joyState _joy_state;
	uint32_t _dev_id;
	k3win32JoyAxisRange _axis_range[K3JOY_MAX_AXES];
	PHIDP_PREPARSED_DATA _pPreParsedData;
	PHIDP_BUTTON_CAPS _pButtonCaps;
	PHIDP_VALUE_CAPS _pValueCaps;
	HANDLE _hid_handle;
	uint32_t _input_buffer_size;
	uint32_t _output_buffer_size;
	char* _input_buffer;
	int8_t* _output_buffer;
	uint32_t _buttons_changed;
	uint32_t _axes_changed;
	OVERLAPPED _ov;
};
#ifdef _MSC_VER
#pragma pack(1)
#endif

struct k2win32PS4Input {
	uint8_t report_id;
	uint8_t x;
	uint8_t y;
	uint8_t z;
	uint8_t r;
	uint8_t b0;
	uint8_t b1;
	uint8_t b2;
	uint8_t u;
	uint8_t v;
	uint8_t reserved0[2];
	uint8_t battery;
	int16_t gx;
	int16_t gy;
	int16_t gz;
	int16_t ax;
	int16_t ay;
	int16_t az;
	uint8_t reserved1[5];
	uint8_t ext;
	uint8_t reserved2[2];
	uint8_t tpad_event;
	uint8_t tpad_auto_inc;
	uint8_t tpad0_touch_id;
	uint8_t tpad0_a;
	uint8_t tpad0_b;
	uint8_t tpad0_c;
	uint8_t tpad1_touch_id;
	uint8_t tpad1_a;
	uint8_t tpad1_b;
	uint8_t tpad1_c;
#ifdef __GNUC__
}__attribute__((__packed__));
#else
};
#endif

#ifdef _MSC_VER
#pragma pack()
#endif

class k3win32PS4JoyObj : public k3win32JoyObj {
public:
	k3win32PS4JoyObj(HANDLE hDevice);
	virtual ~k3win32PS4JoyObj();
	virtual void Poll();
	virtual void SetAttribute(k3joyAttr attr_type, uint32_t num_values, float* values);

	static const uint32_t PRODUCT_ID = 1476;
	static const uint32_t VENDOR_ID = 1356;
};

class k3soundBufImpl
{
public:
	k3soundBufImpl();
	virtual ~k3soundBufImpl();
	static uint32_t _num_sbuf;
	static LPDIRECTSOUND8 _dsound;

	LPDIRECTSOUNDBUFFER8 _buf;
	uint32_t _write_pos;
	uint32_t _buf_size;
	bool _is_playing;
	void* _last_aux;
};

class k3win32WinImpl : public k3winImpl
{
public:
	k3win32WinImpl();
	virtual ~k3win32WinImpl();
	HWND _hwnd;
	HDC _hdc;

	bool _mouse_in_nc;
	static const uint32_t MAX_WIN = 32;
	static const uint32_t MAX_JOY = 32;
	static const uint32_t BACK_BUFFERS = 2;
	static uint32_t _win_count;
	static const char* _win_class_name;
	static HCURSOR _app_cursor;
	static HICON _app_icon;
	static WNDCLASSEX _win_class;
	static const DWORD _windowed_style;
	static k3win _win_map[MAX_WIN];
	static k3win32WinImpl* _winimpl_map[MAX_WIN];
	static uint32_t _num_joy;
	static k3win32Joy _joy_map[MAX_JOY];
	static bool _win32_cursor_visible;
	static STICKYKEYS _start_sticky_keys;
	static TOGGLEKEYS _start_toggle_keys;
	static FILTERKEYS _start_filter_keys;

	static void Initialize();
	static void Uninitialize();
	static void AllowAccesibilityKeys(bool allow);
	static bool PollJoysticks();
	static LRESULT CALLBACK KeyboardLowLevelProc(int ncode, WPARAM wparam, LPARAM lparam);
	static LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static k3key ConvertVKey(uint32_t vkey, uint32_t scan_code, bool extended);
	static void SetWin32CursorState(bool visible);
	void GetFullWindowSize(uint32_t* width, uint32_t* height);
	virtual void ResizeBackBuffer() = 0;
};
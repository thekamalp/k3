// k3 graphics library
// windows joystick class
// Date: 10/10/2021

#include "k3win32.h"

k3win32Joy k3win32JoyObj::Create(HANDLE hDevice, uint32_t dev_id)
{
	k3win32Joy joy;
	RID_DEVICE_INFO dev_info = { 0 };
	uint32_t buf_size = sizeof(RID_DEVICE_INFO);
	GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &dev_info, &buf_size);
	if (dev_info.hid.dwVendorId == k3win32PS4JoyObj::VENDOR_ID &&
		dev_info.hid.dwProductId == k3win32PS4JoyObj::PRODUCT_ID) {
		joy = new k3win32PS4JoyObj(hDevice);
	} else {
		joy = new k3win32JoyObj(hDevice);
	}
	joy->_dev_id = dev_id;
	return joy;
}

k3win32JoyObj::k3win32JoyObj()
{
	InitData();
}

k3win32JoyObj::k3win32JoyObj(HANDLE hDevice)
{
	InitData();

	HANDLE hHeap = GetProcessHeap();
	uint32_t buf_size = 0;
	// Allocate and get pre parsed data
	GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &buf_size);
	_pPreParsedData = static_cast<PHIDP_PREPARSED_DATA>(HeapAlloc(hHeap, 0, buf_size));
	GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, _pPreParsedData, &buf_size);

	// Allocate and get device caps
	uint32_t i;
	HIDP_CAPS Caps;
	NTSTATUS ntstatus;
	ntstatus = HidP_GetCaps(_pPreParsedData, &Caps);
	if (ntstatus != HIDP_STATUS_SUCCESS) {
		k3error::Handler("Could not get HID Caps", "k3win32JoyObj");
		return;
	}
	_pButtonCaps = static_cast<PHIDP_BUTTON_CAPS>(HeapAlloc(hHeap, 0, sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps));

	// Get number of buttons
	uint16_t capsLength = Caps.NumberInputButtonCaps;
	ntstatus = HidP_GetButtonCaps(HidP_Input, _pButtonCaps, &capsLength, _pPreParsedData);
	if (ntstatus != HIDP_STATUS_SUCCESS) {
		k3error::Handler("Could not get HID Button Caps", "k3win32JoyObj");
		return;
	}
	_joy_info.num_buttons = _pButtonCaps->Range.UsageMax - _pButtonCaps->Range.UsageMin + 1;
	if (_joy_info.num_buttons > K3JOY_MAX_BUTTONS) _joy_info.num_buttons = K3JOY_MAX_BUTTONS;

	// Get number of axes
	_pValueCaps = static_cast<PHIDP_VALUE_CAPS>(HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps));
	capsLength = Caps.NumberInputValueCaps;
	ntstatus = HidP_GetValueCaps(HidP_Input, _pValueCaps, &capsLength, _pPreParsedData);
	if (ntstatus != HIDP_STATUS_SUCCESS) {
		k3error::Handler("Could not get HID Value Caps", "k3win32JoyObj");
		return;
	}
	_joy_info.num_axes = Caps.NumberInputValueCaps;
	if (_joy_info.num_axes > K3JOY_MAX_AXES) _joy_info.num_axes = K3JOY_MAX_AXES;

	// Set axis types, and range
	uint32_t axis_ordinal[K3JOY_MAX_AXES] = { 0 };
	for (i = 0; i < _joy_info.num_axes; i++) {
		_joy_info.axis[i] = UsageToAxisType(_pValueCaps[i].UsagePage, _pValueCaps[i].Range.UsageMin);
		_joy_info.axis_ordinal[i] = axis_ordinal[static_cast<uint32_t>(_joy_info.axis[i])];
		axis_ordinal[static_cast<uint32_t>(_joy_info.axis[i])]++;
		_axis_range[i].min = _pValueCaps[i].LogicalMin;
		_axis_range[i].max = _pValueCaps[i].LogicalMax;
	}

	// Create handle to device collection
	char dev_name[256] = { 0 };
	buf_size = 256;
	GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, dev_name, &buf_size);
	_hid_handle = CreateFile(dev_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	// Get product name
	char buf[256];
	buf_size = 256;
	bool success = false;
	success = HidD_GetProductString(_hid_handle, buf, buf_size);
	if (!success) {
		k3error::Handler("Could not get HID Product string Caps", "k3win32JoyObj");
		return;
	}
	for (i = 0; i < 256 && (i < 2 || buf[i - 2] != '\0'); i += 2) _joy_info.name[i / 2] = buf[i];

	// No attributes
	_joy_info.num_attr = 0;

	// Create input and output buffers
	_input_buffer_size = Caps.InputReportByteLength;
	_output_buffer_size = Caps.OutputReportByteLength;
	_input_buffer = static_cast<char*>(HeapAlloc(hHeap, 0, _input_buffer_size));
	_output_buffer = static_cast<int8_t*>(HeapAlloc(hHeap, 0, _output_buffer_size));

	// Clear joystick state
	_buttons_changed = (1 << _joy_info.num_buttons) - 1;
	_axes_changed = (1 << _joy_info.num_axes) - 1;
	_joy_state.buttons_pressed = 0;
	for (i = 0; i < K3JOY_MAX_AXES; i++) {
		_joy_state.axis[i] = 0.0f;
	}

	// Get latest joystick state
	DWORD bytes_written;
	ReadFile(_hid_handle, _input_buffer, _input_buffer_size, &bytes_written, &_ov);
	Poll();
}

void k3win32JoyObj::InitData()
{
	memset(&_joy_info, 0, sizeof(k3joyInfo));
	memset(&_joy_state, 0, sizeof(k3joyState));
	_dev_id = 0;
	memset(_axis_range, 0, K3JOY_MAX_AXES * sizeof(k3win32JoyAxisRange));
	_pPreParsedData = NULL;
	_pButtonCaps = NULL;
	_pValueCaps = NULL;
	_hid_handle = NULL;
	_input_buffer_size = 0;
	_output_buffer_size = 0;
	_input_buffer = NULL;
	_output_buffer = NULL;
	_buttons_changed = 0;
	_axes_changed = 0;
	memset(&_ov, 0, sizeof(OVERLAPPED));
}

k3win32JoyObj::~k3win32JoyObj()
{
	HANDLE hHeap = GetProcessHeap();
	if (_hid_handle)    CloseHandle(_hid_handle);
	if (_input_buffer)   HeapFree(hHeap, 0, _input_buffer);
	if (_output_buffer)  HeapFree(hHeap, 0, _output_buffer);
	if (_pValueCaps)    HeapFree(hHeap, 0, _pValueCaps);
	if (_pButtonCaps)   HeapFree(hHeap, 0, _pButtonCaps);
	if (_pPreParsedData) HeapFree(hHeap, 0, _pPreParsedData);
}

void k3win32JoyObj::Poll()
{
	DWORD bytes_written = 0;

	bool success = GetOverlappedResult(_hid_handle, &_ov, &bytes_written, false);
	if(!success || bytes_written == 0) {
		return;
	}

	if (success) {
		uint32_t i;
		// Get button state
		USAGE usage[K3JOY_MAX_BUTTONS];
		ULONG usage_length = _joy_info.num_buttons;
		uint32_t buttons_pressed = 0x0;
		NTSTATUS ntstatus;
		ntstatus = HidP_GetUsages(HidP_Input, _pButtonCaps->UsagePage, 0, usage, &usage_length, _pPreParsedData, _input_buffer, _input_buffer_size);
		if (ntstatus != HIDP_STATUS_SUCCESS) {
			k3error::Handler("Could not get HID Usages", "Poll");
			return;
		}
		for (i = 0; i < usage_length; i++)
			if (usage[i] >= _pButtonCaps->Range.UsageMin) buttons_pressed |= 1 << (usage[i] - _pButtonCaps->Range.UsageMin);
		_buttons_changed |= buttons_pressed ^ _joy_state.buttons_pressed;
		_joy_state.buttons_pressed = buttons_pressed;
		// Get axis state
		for (i = 0; i < _joy_info.num_axes; i++) {
			ULONG value;
			float fvalue;
			ntstatus = HidP_GetUsageValue(HidP_Input, _pValueCaps[i].UsagePage, 0, _pValueCaps[i].Range.UsageMin, &value, _pPreParsedData, _input_buffer, _input_buffer_size);
			if (ntstatus != HIDP_STATUS_SUCCESS) {
				k3error::Handler("Could not get HID Usage Values", "Poll");
				return;
			}
			fvalue = NormalizeJoystickPos(value, _axis_range[i]);
			if (fvalue != _joy_state.axis[i]) _axes_changed |= (1 << i);
			_joy_state.axis[i] = fvalue;
		}
	}
	ReadFile(_hid_handle, _input_buffer, _input_buffer_size, &bytes_written, &_ov);
}

void k3win32JoyObj::SetAttribute(k3joyAttr attr_type, uint32_t num_values, float* values)
{
}

uint32_t k3win32JoyObj::getDevId()
{
	return _dev_id;
}

const k3joyInfo* k3win32JoyObj::getJoyInfo()
{
	return &_joy_info;
}

const k3joyState* k3win32JoyObj::getJoyState()
{
	return &_joy_state;
}

bool k3win32JoyObj::buttonHasChanged(uint32_t button)
{
	bool ret = (_buttons_changed & (1 << button)) ? true : false;
	_buttons_changed &= ~(1 << button);
	return ret;
}

bool k3win32JoyObj::axisHasChanged(uint32_t axis)
{
	bool ret = (_axes_changed & (1 << axis)) ? true : false;
	_axes_changed &= ~(1 << axis);
	return ret;
}

float k3win32JoyObj::NormalizeJoystickPos(ULONG pos, k3win32JoyAxisRange range)
{
	float scale = 1.0f / (range.max - range.min);
	float fpos = (pos - range.min) * scale;
	return fpos;
}

k3joyAxis k3win32JoyObj::UsageToAxisType(USAGE page, USAGE usage)
{
	k3joyAxis axis_type = k3joyAxis::UNKNOWN;

	if (page == 0x01) {
		switch (usage) {
		case 0x30: axis_type = k3joyAxis::X; break;
		case 0x31: axis_type = k3joyAxis::Y; break;
		case 0x32: axis_type = k3joyAxis::Z; break;
		case 0x35: axis_type = k3joyAxis::R; break;
		case 0x33: axis_type = k3joyAxis::U; break;
		case 0x34: axis_type = k3joyAxis::V; break;
		case 0x39: axis_type = k3joyAxis::POV; break;
		default:   axis_type = k3joyAxis::UNKNOWN; break;
		}
	}

	return axis_type;
}


k3win32PS4JoyObj::k3win32PS4JoyObj(HANDLE hDevice)
{
	// Initialize joy info structure
	_joy_info.num_axes = 17;
	_joy_info.num_buttons = 16;
	_joy_info.axis[0] = k3joyAxis::X; _joy_info.axis_ordinal[0] = 0;
	_joy_info.axis[1] = k3joyAxis::Y; _joy_info.axis_ordinal[1] = 0;
	_joy_info.axis[2] = k3joyAxis::X; _joy_info.axis_ordinal[2] = 1;
	_joy_info.axis[3] = k3joyAxis::Y; _joy_info.axis_ordinal[3] = 1;
	_joy_info.axis[4] = k3joyAxis::U; _joy_info.axis_ordinal[4] = 0;
	_joy_info.axis[5] = k3joyAxis::V; _joy_info.axis_ordinal[5] = 0;
	_joy_info.axis[6] = k3joyAxis::POV; _joy_info.axis_ordinal[6] = 0;
	_joy_info.axis[7] = k3joyAxis::GX; _joy_info.axis_ordinal[7] = 0;
	_joy_info.axis[8] = k3joyAxis::GY; _joy_info.axis_ordinal[8] = 0;
	_joy_info.axis[9] = k3joyAxis::GZ; _joy_info.axis_ordinal[9] = 0;
	_joy_info.axis[10] = k3joyAxis::AX; _joy_info.axis_ordinal[10] = 0;
	_joy_info.axis[11] = k3joyAxis::AY; _joy_info.axis_ordinal[11] = 0;
	_joy_info.axis[12] = k3joyAxis::AZ; _joy_info.axis_ordinal[12] = 0;
	_joy_info.axis[13] = k3joyAxis::TPAD_X; _joy_info.axis_ordinal[13] = 0;
	_joy_info.axis[14] = k3joyAxis::TPAD_Y; _joy_info.axis_ordinal[14] = 0;
	_joy_info.axis[15] = k3joyAxis::TPAD_X; _joy_info.axis_ordinal[15] = 1;
	_joy_info.axis[16] = k3joyAxis::TPAD_Y; _joy_info.axis_ordinal[16] = 1;

	// Create handle to device collection
	char dev_name[256] = { 0 };
	uint32_t buf_size = 256;
	GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, dev_name, &buf_size);
	_hid_handle = CreateFile(dev_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);

	// Get product name
	uint32_t i;
	//char buf[256];
	//buf_size = 256;
	//uint32_t ret = HidD_GetProductString( _hid_handle, buf, buf_size );
	//for( i=0; i<256 && (i>1 || buf[i-2] != '\0'); i+=2 ) _joy_info.name[i/2] = buf[i];
	strncpy_s(_joy_info.name, "DualShock4", 128);

	// Attributes
	_joy_info.num_attr = 2;
	_joy_info.attr_values[0] = 2;
	_joy_info.attr_type[0] = k3joyAttr::RUMBLE;
	_joy_info.attr_values[1] = 3;
	_joy_info.attr_type[1] = k3joyAttr::LIGHT_BAR;

	// Create input/output buffers
	_input_buffer_size = 64;
	_output_buffer_size = 32;
	HANDLE hHeap = GetProcessHeap();
	_input_buffer = static_cast<char*>(HeapAlloc(hHeap, 0, _input_buffer_size));
	_output_buffer = static_cast<int8_t*>(HeapAlloc(hHeap, 0, _output_buffer_size));

	// Clear joystick state
	_buttons_changed = (1 << _joy_info.num_buttons) - 1;
	_axes_changed = (1 << _joy_info.num_axes) - 1;
	_joy_state.buttons_pressed = 0;
	for (i = 0; i < K3JOY_MAX_AXES; i++) {
		_joy_state.axis[i] = 0.0f;
	}

	// Get latest joystick state
	Poll();
}

k3win32PS4JoyObj::~k3win32PS4JoyObj()
{
	// Set all attributes to 0
	float zeros[32] = { 0.0f };
	uint32_t i;
	for (i = 0; i < _joy_info.num_attr; i++) {
		k3win32PS4JoyObj::SetAttribute(_joy_info.attr_type[i], _joy_info.attr_values[i], zeros);
	}
}

void k3win32PS4JoyObj::Poll()
{
	DWORD bytes_written = 0;
	if (ReadFile(_hid_handle, _input_buffer, _input_buffer_size, &bytes_written, NULL)) {
		k2win32PS4Input* data = reinterpret_cast<k2win32PS4Input*>(_input_buffer);
		// Get button state
		uint32_t buttons_pressed;
		buttons_pressed = data->b0 >> 4;
		buttons_pressed |= data->b1 << 4;
		buttons_pressed |= (data->b2 & 3) << 12;
		if ((data->tpad0_touch_id & 0x80) == 0) buttons_pressed |= (1 << 14);
		if ((data->tpad1_touch_id & 0x80) == 0) buttons_pressed |= (1 << 15);
		_buttons_changed |= buttons_pressed ^ _joy_state.buttons_pressed;
		_joy_state.buttons_pressed = buttons_pressed;
		// Get axis state
		float value;
		value = data->x / 255.0f;
		if (value != _joy_state.axis[0]) _axes_changed |= (1 << 0);
		_joy_state.axis[0] = value;
		value = data->y / 255.0f;
		if (value != _joy_state.axis[1]) _axes_changed |= (1 << 1);
		_joy_state.axis[1] = value;
		value = data->z / 255.0f;
		if (value != _joy_state.axis[2]) _axes_changed |= (1 << 2);
		_joy_state.axis[2] = value;
		value = data->r / 255.0f;
		if (value != _joy_state.axis[3]) _axes_changed |= (1 << 3);
		_joy_state.axis[3] = value;
		value = data->u / 255.0f;
		if (value != _joy_state.axis[4]) _axes_changed |= (1 << 4);
		_joy_state.axis[4] = value;
		value = data->v / 255.0f;
		if (value != _joy_state.axis[5]) _axes_changed |= (1 << 5);
		_joy_state.axis[5] = value;
		value = (data->b0 & 0xf) / 8.0f;
		if (value != _joy_state.axis[6]) _axes_changed |= (1 << 6);
		_joy_state.axis[6] = value;
		// Following measures angular acceleration in degrees per second
		value = data->gx * (2000.0f / 32767.0f);
		if (value != _joy_state.axis[7]) _axes_changed |= (1 << 7);
		_joy_state.axis[7] = value;
		value = data->gy * (2000.0f / 32767.0f);
		if (value != _joy_state.axis[8]) _axes_changed |= (1 << 8);
		_joy_state.axis[8] = value;
		value = data->gz * (2000.0f / 32767.0f);
		if (value != _joy_state.axis[9]) _axes_changed |= (1 << 9);
		_joy_state.axis[9] = value;
		// the following measures acceleration in g forces
		value = data->ax / 8192.0f;
		if (value != _joy_state.axis[10]) _axes_changed |= (1 << 10);
		_joy_state.axis[10] = value;
		value = data->ay / 8192.0f;
		if (value != _joy_state.axis[11]) _axes_changed |= (1 << 11);
		_joy_state.axis[11] = value;
		value = data->az / 8192.0f;
		if (value != _joy_state.axis[12]) _axes_changed |= (1 << 12);
		_joy_state.axis[12] = value;
		value = (data->tpad0_a | ((data->tpad0_b & 0xf) << 8)) / 1919.0f;
		if (value != _joy_state.axis[13]) _axes_changed |= (1 << 13);
		_joy_state.axis[13] = value;
		value = ((data->tpad0_b >> 4) | (data->tpad0_c << 4)) / 942.0f;
		if (value != _joy_state.axis[14]) _axes_changed |= (1 << 14);
		_joy_state.axis[14] = value;
		value = (data->tpad1_a | ((data->tpad1_b & 0xf) << 8)) / 1919.0f;
		if (value != _joy_state.axis[15]) _axes_changed |= (1 << 15);
		_joy_state.axis[15] = value;
		value = ((data->tpad1_b >> 4) | (data->tpad1_c << 4)) / 942.0f;
		if (value != _joy_state.axis[16]) _axes_changed |= (1 << 16);
		_joy_state.axis[16] = value;
	}
}

void k3win32PS4JoyObj::SetAttribute(k3joyAttr attr_type, uint32_t num_values, float* values)
{
	memset(_output_buffer, 0, _output_buffer_size);
	_output_buffer[0] = 0x5;
	uint8_t* buf = reinterpret_cast<uint8_t*>(_output_buffer);
	switch (attr_type) {
	case k3joyAttr::RUMBLE:
		if (num_values >= 2) {
			buf[1] = 0xf1;
			buf[4] = static_cast<uint8_t>(values[0] * 255);
			buf[5] = static_cast<uint8_t>(values[1] * 255);
		}
		break;
	case k3joyAttr::LIGHT_BAR:
		if (num_values >= 3) {
			buf[1] = 0xff & ~0xf1;
			buf[6] = static_cast<uint8_t>(values[0] * 255);
			buf[7] = static_cast<uint8_t>(values[1] * 255);
			buf[8] = static_cast<uint8_t>(values[2] * 255);
		}
		break;
	default:
		break;
	}
	if (_output_buffer[1] != 0x0) {
		DWORD bytes_written = 0;
		WriteFile(_hid_handle, _output_buffer, _output_buffer_size, &bytes_written, NULL);
	}
}
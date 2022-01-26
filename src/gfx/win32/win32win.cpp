// k3 graphics library
// windows win32 class
// Date: 10/10/2021

#include "k3win32.h"

uint32_t k3win32WinImpl::_win_count = 0;
const char* k3win32WinImpl::_win_class_name = "k3window";
HCURSOR k3win32WinImpl::_app_cursor;
HICON k3win32WinImpl::_app_icon;
WNDCLASSEX k3win32WinImpl::_win_class;
const DWORD k3win32WinImpl::_windowed_style = WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;;
k3win k3win32WinImpl::_win_map[MAX_WIN] = { NULL };
k3win32WinImpl* k3win32WinImpl::_winimpl_map[MAX_WIN] = { NULL };
uint32_t k3win32WinImpl::_num_joy = 0;
k3win32Joy k3win32WinImpl::_joy_map[MAX_JOY] = { NULL };
bool k3win32WinImpl::_win32_cursor_visible = true;

uint32_t k3soundBufImpl::_num_sbuf = 0;
LPDIRECTSOUND8 k3soundBufImpl::_dsound = NULL;

// ------------------------------------------------------------
// sound buffer
k3soundBufImpl::k3soundBufImpl()
{
    _buf = NULL;
    _write_pos = 0;
    _buf_size = 0;
    _is_playing = false;
    _last_aux = NULL;

    if (_dsound == NULL) {
        HRESULT hr;
        hr = DirectSoundCreate8(NULL, &_dsound, NULL);
        if (FAILED(hr)) {
            _dsound = NULL;
        }
    }
    _num_sbuf++;
}

k3soundBufImpl::~k3soundBufImpl()
{
    if (_buf) {
        _buf->Release();
        _buf = NULL;
    }
    _num_sbuf--;
    if (_num_sbuf == 0 && _dsound != NULL) {
        _dsound->Release();
        _dsound = NULL;
    }
}

k3soundBufObj::k3soundBufObj()
{
    _data = new k3soundBufImpl;
}

k3soundBufObj::~k3soundBufObj()
{
    if (_data) {
        delete _data;
        _data = NULL;
    }
}

k3soundBufImpl* k3soundBufObj::getImpl()
{
    return _data;
}

const k3soundBufImpl* k3soundBufObj::getImpl() const
{
    return _data;
}

K3API void k3soundBufObj::UpdateSBuffer(uint32_t offset, const void* data, uint32_t size)
{
    LPVOID p0, p1;
    DWORD size0, size1;

    if (_data->_buf == NULL) return;
    if (offset != 0xFFFFFFFF) _data->_write_pos = offset % _data->_buf_size;

    HRESULT hr;
    hr = _data->_buf->Lock(_data->_write_pos, size, &p0, &size0, &p1, &size1, 0);
    if (hr == DSERR_BUFFERLOST) {
        _data->_buf->Restore();
        hr = _data->_buf->Lock(_data->_write_pos, size, &p0, &size0, &p1, &size1, 0);
    }
    if (SUCCEEDED(hr)) {
        memcpy(p0, data, size0);
        if (p1) {
            const void* s1 = static_cast<const void*>(static_cast<const uint8_t*>(data) + size0);
            memcpy(p1, s1, size1);
            _data->_write_pos = size1;
        } else {
            _data->_write_pos += size0;
            _data->_write_pos = _data->_write_pos % _data->_buf_size;
        }
        _data->_buf->Unlock(p0, size0, p1, size1);
    }
}

K3API void* k3soundBufObj::MapForWrite(uint32_t offset, uint32_t size, void** aux, uint32_t* aux_size)
{
    LPVOID p0 = NULL, p1 = NULL;
    DWORD size0 = 0, size1 = 0;

    if (_data->_buf == NULL) return NULL;
    if (offset != 0xFFFFFFFF) _data->_write_pos = offset % _data->_buf_size;

    HRESULT hr;
    hr = _data->_buf->Lock(_data->_write_pos, size, &p0, &size0, &p1, &size1, 0);
    if (hr == DSERR_BUFFERLOST) {
        _data->_buf->Restore();
        hr = _data->_buf->Lock(_data->_write_pos, size, &p0, &size0, &p1, &size1, 0);
    }
    if (SUCCEEDED(hr)) {
        if (aux) *aux = p1;
        if (aux_size) *aux_size = size1;
        _data->_last_aux = p1;
    }
    return p0;
}

K3API void k3soundBufObj::Unmap(void* p, uint32_t offset, uint32_t size)
{
    uint32_t size0 = size;
    if (offset + size > _data->_buf_size) size0 = _data->_buf_size - offset;
    uint32_t size1 = size - size0;
    _data->_buf->Unlock(p, size0, _data->_last_aux, size1);
}

K3API void k3soundBufObj::PlaySBuffer(uint32_t offset)
{
    if (_data->_buf == NULL) return;
    if (offset != 0xFFFFFFFF) {
        _data->_buf->SetCurrentPosition(offset % _data->_buf_size);
        _data->_buf->Play(0, 0, DSBPLAY_LOOPING);
    } else if (!_data->_is_playing) {
        _data->_buf->Play(0, 0, DSBPLAY_LOOPING);
    }
    _data->_is_playing = true;
}

K3API uint32_t k3soundBufObj::GetPlayPosition()
{
    if (_data->_buf == NULL) return 0;
    DWORD cur_pos;//, wr_pos;
    _data->_buf->GetCurrentPosition(&cur_pos, NULL);//&wr_pos);
    //printf("play_pos: %f wr_pos: %f\n", cur_pos / 1470.0f, wr_pos / 1470.0f);
    return cur_pos;
}

K3API uint32_t k3soundBufObj::GetWritePosition()
{
    return _data->_write_pos;
}

K3API void k3soundBufObj::StopSBuffer()
{
    _data->_is_playing = false;
    if (_data->_buf == NULL) return;
    _data->_buf->Stop();
}

// ------------------------------------------------------------
// timer

K3API void k3timerObj::Sleep(uint32_t time)
{
    ::Sleep(time);
}

uint32_t k3timerObj::getSystemTime()
{
    return timeGetTime();
}

// ------------------------------------------------------------
// windows

k3win32WinImpl::k3win32WinImpl()
{
    _hwnd = NULL;
    _hdc = NULL;
    _mouse_in_nc = false;
}

k3win32WinImpl::~k3win32WinImpl()
{ }

void k3win32WinImpl::Initialize()
{
    _app_cursor = LoadCursor(NULL, IDC_ARROW);
    _app_icon = LoadIcon(GetModuleHandle(NULL), "IDI_MAIN");
    if (_app_icon == NULL)
        _app_icon = LoadIcon(NULL, IDI_APPLICATION);

    _win_class.cbSize = sizeof(WNDCLASSEX);
    _win_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    _win_class.lpfnWndProc = k3win32WinImpl::MsgProc;
    _win_class.cbClsExtra = 0L;
    _win_class.cbWndExtra = 0L;
    _win_class.hInstance = GetModuleHandle(NULL);
    _win_class.hIcon = _app_icon;
    _win_class.hCursor = _app_cursor;
    _win_class.hbrBackground = NULL;
    _win_class.lpszMenuName = NULL;
    _win_class.lpszClassName = k3win32WinImpl::_win_class_name;
    _win_class.hIconSm = NULL;

    RegisterClassEx(&_win_class);

}

void k3win32WinImpl::Uninitialize()
{
    // Unregister input devices
    RAWINPUTDEVICE rid[1];

    // For joysticks
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x05;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = NULL; // follow keyboard focus
    RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE));

    UnregisterClass(k3win32WinImpl::_win_class_name, GetModuleHandle(NULL));
}

bool k3win32WinImpl::PollJoysticks()
{
	bool have_joy = false;
	k3win32WinImpl* winimpl;
	k3win32Joy joy;
	uint32_t w, j;

	// if we have no joysticks, return false
	if (_num_joy == 0) return false;

	// Loop through the windows, and see if any has a joystick callback function
	for (w = 0; w < _win_count; w++) {
		winimpl = _winimpl_map[w];
		if (winimpl->JoystickButton != NULL || winimpl->JoystickMove != NULL) have_joy = true;
	}
	if (!have_joy) return false;

	// Loop through each joystick
	for (j = 0; j < _num_joy; j++) {
		joy = _joy_map[j];
		joy->Poll();
		const k3joyInfo* joyinfo = joy->getJoyInfo();
		const k3joyState* joystate = joy->getJoyState();
		uint32_t b;
		// Loop through each button
		for (b = 0; b < joyinfo->num_buttons; b++) {
			if (joy->buttonHasChanged(b)) {
				for (w = 0; w < _win_count; w++) {
					winimpl = _winimpl_map[w];
					if (winimpl->JoystickButton != NULL)
						winimpl->JoystickButton(winimpl->_data, joy->getDevId(), b, (joystate->buttons_pressed & (1 << b)) ? k3keyState::PRESSED : k3keyState::RELEASED);
				}
			}
		}
		uint32_t a;
		// Loop through each axis
		for (a = 0; a < joyinfo->num_axes; a++) {
			if (joy->axisHasChanged(a)) {
				for (w = 0; w < _win_count; w++) {
					winimpl = _winimpl_map[w];
					if (winimpl->JoystickMove != NULL)
						winimpl->JoystickMove(winimpl->_data, joy->getDevId(), a, joyinfo->axis[a], joyinfo->axis_ordinal[a], joystate->axis[a]);
				}
			}
		}
	}

	return true;
}

LRESULT CALLBACK k3win32WinImpl::KeyboardLowLevelProc(int ncode, WPARAM wparam, LPARAM lparam)
{
    static bool sysreq_pressed = false;
    bool keyprocessed = false;

    if (ncode == HC_ACTION) {
        switch (wparam) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            PKBDLLHOOKSTRUCT p = reinterpret_cast<PKBDLLHOOKSTRUCT>(lparam);
            if (p->vkCode == VK_SNAPSHOT) {
                LPARAM new_lparam = (p->scanCode) << 16;
                if (p->flags & LLKHF_EXTENDED) new_lparam |= (1 << 24);
                if (p->flags & LLKHF_ALTDOWN) new_lparam |= (1 << 29);
                if (p->flags & LLKHF_UP) {
                    new_lparam |= (1 << 31);
                    sysreq_pressed = false;
                } else {
                    if (sysreq_pressed) new_lparam |= (1 << 30);
                    sysreq_pressed = true;
                }
                PostMessage(GetForegroundWindow(), wparam, p->vkCode, new_lparam);
                keyprocessed = true;
            }
            break;
        }
    }

    return keyprocessed ? 1 : CallNextHookEx(NULL, ncode, wparam, lparam);
}

LRESULT WINAPI k3win32WinImpl::MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    k3win32WinImpl* winimpl = NULL;
    uint32_t w;
    for (w = 0; w < _win_count; w++) {
        if (_winimpl_map[w]->_hwnd == hwnd) {
            winimpl = _winimpl_map[w];
            break;
        }
    }

    uint32_t x = static_cast<unsigned short>(lparam & 0xffff);
    uint32_t y = static_cast<unsigned short>((lparam >> 16) & 0xffff);

    if (winimpl) {
        switch (msg) {
        case WM_PAINT:
            if (winimpl->Display) winimpl->Display(winimpl->_data);
            break;
        case WM_CLOSE:
            if (winimpl->Destroy) winimpl->Destroy(winimpl->_data);
            _winimpl_map[w] = _winimpl_map[_win_count - 1];
            _win_map[w] = _win_map[_win_count - 1];
            _winimpl_map[_win_count - 1] = NULL;
            _win_map[_win_count - 1] = NULL;
            _win_count--;
            if (_win_count == 0) k3winObj::ExitLoop();
            break;
        case WM_INPUT_DEVICE_CHANGE:
            if (w == 0) { // Ensures we only do this once, not for each window
                HANDLE hDevice = reinterpret_cast<HANDLE>(lparam);
                uint32_t dev_id = lparam;
                if (wparam == GIDC_ARRIVAL && _num_joy < MAX_JOY) {
                    _joy_map[_num_joy] = k3win32JoyObj::Create(hDevice, dev_id);
                    uint32_t w2;
                    for (w2 = 0; w2 < _win_count; w2++) {
                        if (_winimpl_map[w2]->JoystickAdded) _winimpl_map[w2]->JoystickAdded(_winimpl_map[w2]->_data, dev_id, _joy_map[_num_joy]->getJoyInfo(), _joy_map[_num_joy]->getJoyState());
                    }
                    _num_joy++;
                }
                if (wparam == GIDC_REMOVAL) {
                    uint32_t j;
                    for (j = 0; j < _num_joy; j++) {
                        if (_joy_map[j]->getDevId() == dev_id) break;
                    }
                    if (j < _num_joy) {
                        uint32_t w2;
                        for (w2 = 0; w2 < _win_count; w2++) {
                            if (_winimpl_map[w2]->JoystickRemoved) _winimpl_map[w2]->JoystickRemoved(_winimpl_map[w2]->_data, dev_id);
                        }
                        _num_joy--;
                        _joy_map[j] = _joy_map[_num_joy];
                        _joy_map[_num_joy] = NULL;
                    }
                }
            }
            break;
            //case WM_DEVICECHANGE:
            //  InitializeJoysticks();
            //  break;
        case WM_NCMOUSEMOVE:
            SetWin32CursorState(true);
            winimpl->_mouse_in_nc = true;
            break;
        case WM_MOUSEMOVE:
            SetWin32CursorState(winimpl->_is_cursor_visible);
            winimpl->_mouse_in_nc = false;
            if (winimpl->MouseMove) winimpl->MouseMove(winimpl->_data, x, y);
            break;
        case WM_LBUTTONDOWN:
            if (winimpl->MouseButton) winimpl->MouseButton(winimpl->_data, x, y, 1, k3keyState::PRESSED);
            break;
        case WM_LBUTTONUP:
            if (winimpl->MouseButton) winimpl->MouseButton(winimpl->_data, x, y, 1, k3keyState::RELEASED);
            break;
        case WM_MBUTTONDOWN:
            if (winimpl->MouseButton) winimpl->MouseButton(winimpl->_data, x, y, 2, k3keyState::PRESSED);
            break;
        case WM_MBUTTONUP:
            if (winimpl->MouseButton) winimpl->MouseButton(winimpl->_data, x, y, 2, k3keyState::RELEASED);
            break;
        case WM_RBUTTONDOWN:
            if (winimpl->MouseButton) winimpl->MouseButton(winimpl->_data, x, y, 3, k3keyState::PRESSED);
            break;
        case WM_RBUTTONUP:
            if (winimpl->MouseButton) winimpl->MouseButton(winimpl->_data, x, y, 3, k3keyState::RELEASED);
            break;
        case WM_MOUSEWHEEL:
            if (winimpl->MouseScroll) {
                int16_t zdelta = static_cast<int16_t>(wparam >> 16);
                zdelta = (zdelta < 0) ? 1 : -1;
                winimpl->MouseScroll(winimpl->_data, x, y, zdelta, 0);
            }
            break;
        case WM_MOUSEHWHEEL:
            if (winimpl->MouseScroll) {
                int16_t zdelta = static_cast<int16_t>(wparam >> 16);
                zdelta = (zdelta > 0) ? 1 : -1;
                winimpl->MouseScroll(winimpl->_data, x, y, 0, zdelta);
            }
            break;
        case WM_SIZE:
            if (winimpl->_width != x || winimpl->_height != y) {
                winimpl->_width = x;
                winimpl->_height = y;
                winimpl->ResizeBackBuffer();
                if (winimpl->Resize) winimpl->Resize(winimpl->_data, x, y);
            }
            break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (winimpl->Keyboard) {
                uint8_t keys[256];
                WORD c;
                int32_t len;
                k3keyState keystate = k3keyState::NONE;
                bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                bool repeat = (lparam & 0x40000000) ? true : false;
                uint32_t scan_code = (lparam >> 16) & 0xff;
                bool extended = (lparam & 0x1000000) ? true : false;
                k3key k = ConvertVKey(wparam, scan_code, extended);

                if (pressed && repeat) keystate = k3keyState::REPEATED;
                else                  keystate = (pressed) ? k3keyState::PRESSED : k3keyState::RELEASED;

                bool success = GetKeyboardState(keys);
                if (!success) {
                    k3error::Handler("Coudl not get keyboard state", "MsgProc");
                    return 0;
                }
                len = ToAscii(wparam, scan_code, keys, &c, 0);
                if (len != 1) c = '\0';

                winimpl->Keyboard(winimpl->_data, k, static_cast<char>(c), keystate);

                return 0;
            }
            break;
        }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

k3key k3win32WinImpl::ConvertVKey(uint32_t vkey, uint32_t scan_code, bool extended)
{
    k3key k = k3key::NONE;
    uint32_t vkey_lr = MapVirtualKey(scan_code, 3);

    switch (vkey) {
    case VK_ESCAPE: k = k3key::ESCAPE; break;
    case VK_F1: k = k3key::F1; break;
    case VK_F2: k = k3key::F2; break;
    case VK_F3: k = k3key::F3; break;
    case VK_F4: k = k3key::F4; break;
    case VK_F5: k = k3key::F5; break;
    case VK_F6: k = k3key::F6; break;
    case VK_F7: k = k3key::F7; break;
    case VK_F8: k = k3key::F8; break;
    case VK_F9: k = k3key::F9; break;
    case VK_F10: k = k3key::F10; break;
    case VK_F11: k = k3key::F11; break;
    case VK_F12: k = k3key::F12; break;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT: k = (vkey_lr == VK_RSHIFT) ? k3key::RSHIFT : k3key::LSHIFT; break;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL: k = (extended) ? k3key::RCONTROL : k3key::LCONTROL; break;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU: k = (extended) ? k3key::RALT : k3key::LALT; break;
    case VK_LWIN: k = k3key::LWIN; break;
    case VK_RWIN: k = k3key::RWIN; break;
    case VK_APPS: k = k3key::MENU; break;
    case VK_OEM_3: k = k3key::BACKTICK; break;
    case VK_OEM_MINUS: k = k3key::MINUS; break;
    case VK_OEM_PLUS: k = k3key::PLUS; break;
    case VK_BACK: k = k3key::BACKSPACE; break;
    case VK_TAB: k = k3key::TAB; break;
    case VK_OEM_4: k = k3key::LBRACKET; break;
    case VK_OEM_6: k = k3key::RBRACKET; break;
    case VK_OEM_5: k = k3key::BACKSLASH; break;
    case VK_CAPITAL: k = k3key::CAPS_LOCK; break;
    case VK_OEM_1: k = k3key::SEMICOLON; break;
    case VK_OEM_7: k = k3key::TICK; break;
    case VK_RETURN: k = (extended) ? k3key::NUM_ENTER : k3key::ENTER; break;
    case VK_SPACE: k = k3key::SPACE; break;
    case VK_SNAPSHOT: k = k3key::SYS_REQ; break;
    case VK_SCROLL: k = k3key::SCROLL_LOCK; break;
    case VK_PAUSE: k = k3key::PAUSE; break;
    case VK_INSERT: k = k3key::INSERT; break;
    case VK_DELETE: k = (extended) ? k3key::DEL : k3key::NUM_DECIMAL; break;
    case VK_HOME: k = k3key::HOME; break;
    case VK_END: k = k3key::END; break;
    case VK_PRIOR: k = k3key::PAGE_UP; break;
    case VK_NEXT: k = k3key::PAGE_DOWN; break;
    case VK_UP: k = k3key::UP; break;
    case VK_DOWN: k = k3key::DOWN; break;
    case VK_LEFT: k = k3key::LEFT; break;
    case VK_RIGHT: k = k3key::RIGHT; break;
    case VK_NUMLOCK: k = k3key::NUM_LOCK; break;
    case VK_NUMPAD0: k = k3key::NUM_0; break;
    case VK_NUMPAD1: k = k3key::NUM_1; break;
    case VK_NUMPAD2: k = k3key::NUM_2; break;
    case VK_NUMPAD3: k = k3key::NUM_3; break;
    case VK_NUMPAD4: k = k3key::NUM_4; break;
    case VK_NUMPAD5: k = k3key::NUM_5; break;
    case VK_NUMPAD6: k = k3key::NUM_6; break;
    case VK_NUMPAD7: k = k3key::NUM_7; break;
    case VK_NUMPAD8: k = k3key::NUM_8; break;
    case VK_NUMPAD9: k = k3key::NUM_9; break;
    case VK_ADD: k = k3key::NUM_PLUS; break;
    case VK_SUBTRACT: k = k3key::NUM_MINUS; break;
    case VK_MULTIPLY: k = k3key::NUM_TIMES; break;
    case VK_DIVIDE: k = k3key::NUM_DIVIDE; break;
    case 'A': k = k3key::A; break;
    case 'B': k = k3key::B; break;
    case 'C': k = k3key::C; break;
    case 'D': k = k3key::D; break;
    case 'E': k = k3key::E; break;
    case 'F': k = k3key::F; break;
    case 'G': k = k3key::G; break;
    case 'H': k = k3key::H; break;
    case 'I': k = k3key::I; break;
    case 'J': k = k3key::J; break;
    case 'K': k = k3key::K; break;
    case 'L': k = k3key::L; break;
    case 'M': k = k3key::M; break;
    case 'N': k = k3key::N; break;
    case 'O': k = k3key::O; break;
    case 'P': k = k3key::P; break;
    case 'Q': k = k3key::Q; break;
    case 'R': k = k3key::R; break;
    case 'S': k = k3key::S; break;
    case 'T': k = k3key::T; break;
    case 'U': k = k3key::U; break;
    case 'V': k = k3key::V; break;
    case 'W': k = k3key::W; break;
    case 'X': k = k3key::X; break;
    case 'Y': k = k3key::Y; break;
    case 'Z': k = k3key::Z; break;
    case '0': k = k3key::KEY_0; break;
    case '1': k = k3key::KEY_1; break;
    case '2': k = k3key::KEY_2; break;
    case '3': k = k3key::KEY_3; break;
    case '4': k = k3key::KEY_4; break;
    case '5': k = k3key::KEY_5; break;
    case '6': k = k3key::KEY_6; break;
    case '7': k = k3key::KEY_7; break;
    case '8': k = k3key::KEY_8; break;
    case '9': k = k3key::KEY_9; break;
    }
    return k;
}

void k3win32WinImpl::SetWin32CursorState(bool visible)
{
    _win32_cursor_visible = visible;
    ShowCursor(_win32_cursor_visible);
}

void k3win32WinImpl::GetFullWindowSize(uint32_t* width, uint32_t* height)
{
    if (_is_fullscreen) {
        *width = _width;
        *height = _height;
    } else {
        RECT clientR, winR;
        GetClientRect(_hwnd, &clientR);
        GetWindowRect(_hwnd, &winR);
        *width = _width + (winR.right - winR.left) - clientR.right;
        *height = _height + (winR.bottom - winR.top) - clientR.bottom;
    }
}

K3API k3win k3winObj::Create(const char* title,
    uint32_t x, uint32_t y, uint32_t width, uint32_t height,
    k3fmt color_format, bool fullscreen,
    uint32_t num_views, uint32_t num_samplers, k3gfx gfx)
{
    if (k3win32WinImpl::_win_count == 0) k3win32WinImpl::Initialize();
    k3win win = new k3winObj;
    k3win32WinImpl* d = static_cast<k3win32WinImpl*>(win->_data);
    DWORD style = (d->_is_fullscreen) ? WS_POPUP : d->_windowed_style;

    d->_title = title;
    d->_x_pos = x;
    d->_y_pos = y;
    d->_width = width;
    d->_height = height;
    d->_color_fmt = color_format;
    d->_is_fullscreen = fullscreen;

    d->_hwnd = CreateWindowEx(0, k3win32WinImpl::_win_class_name, k3win32WinImpl::_win_class_name,
        style, d->_x_pos, d->_y_pos,
        d->_width, d->_height, NULL, NULL,
        k3win32WinImpl::_win_class.hInstance, NULL);
    
    if (gfx != NULL) {
        d->gfx = gfx;
    } else {
        d->gfx = k3gfxObj::Create(num_views, num_samplers);
    }

    win->SetSize(d->_width, d->_height);

    if (d->_is_fullscreen) win->SetVisible(true);

    if (d->_hwnd == NULL) {
        k3error::Handler("Could not create window", "Create");
        win = NULL;
        return NULL;
    } else {
        d->_hdc = GetDC(d->_hwnd);
        SetWindowText(d->_hwnd, d->_title);
        k3win32WinImpl::_win_map[k3win32WinImpl::_win_count] = win;
        k3win32WinImpl::_winimpl_map[k3win32WinImpl::_win_count] = d;
        k3win32WinImpl::_win_count++;
    }

    // Register input devices
    RAWINPUTDEVICE rid[2];

    // For joysticks
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x05;
    rid[0].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    rid[0].hwndTarget = d->_hwnd;
    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x04;
    rid[1].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    rid[1].hwndTarget = d->_hwnd;
    RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));

    //ShowWindow( _hwnd, ((_is_visible) ? SW_SHOWDEFAULT : SW_HIDE) );
    ShowWindow(d->_hwnd, SW_SHOWDEFAULT);
    d->SetWin32CursorState(d->_is_cursor_visible);

    d->_mouse_in_nc = false;

    d->ResizeBackBuffer();

    return win;
}



K3API void k3winObj::Destroy(k3win win)
{
    uint32_t w;
    for (w = 0; w < k3win32WinImpl::_win_count; w++) {
        if (k3win32WinImpl::_win_map[w] == win) {
            break;
        }
    }
    if (w < k3win32WinImpl::_win_count) {
        k3win32WinImpl::_win_count--;
        k3win32WinImpl::_winimpl_map[w] = k3win32WinImpl::_winimpl_map[k3win32WinImpl::_win_count];
        k3win32WinImpl::_win_map[w] = k3win32WinImpl::_win_map[k3win32WinImpl::_win_count];
        k3win32WinImpl::_winimpl_map[k3win32WinImpl::_win_count] = NULL;
        k3win32WinImpl::_win_map[k3win32WinImpl::_win_count] = NULL;
        if (k3win32WinImpl::_win_count == 0) k3win32WinImpl::Uninitialize();
    }
}

K3API void k3winObj::SetTitle(const char* title)
{
    k3win32WinImpl* d = static_cast<k3win32WinImpl*>(_data);
    if (d->_hwnd) SetWindowText(d->_hwnd, title);
    d->_title = title;
}

K3API void k3winObj::SetSize(uint32_t width, uint32_t height)
{
    k3win32WinImpl* d = static_cast<k3win32WinImpl*>(_data);
    d->_width = width;
    d->_height = height;

    if (d->_hwnd) {
        uint32_t full_width, full_height;
        d->GetFullWindowSize(&full_width, &full_height);
        SetWindowPos(d->_hwnd, NULL, 0, 0, full_width, full_height,
            SWP_NOMOVE | SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
        RECT r;
        GetClientRect(d->_hwnd, &r);
        d->_width = r.right - r.left;
        d->_height = r.bottom - r.top;
    }
}

K3API void k3winObj::SetPosition(uint32_t x, uint32_t y)
{
    k3win32WinImpl* d = static_cast<k3win32WinImpl*>(_data);
    d->_x_pos = x;
    d->_y_pos = y;

    if (d->_hwnd) {
        SetWindowPos(d->_hwnd, NULL, x, y, 0, 0,
            SWP_NOSIZE | SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

K3API void k3winObj::SetCursorPosition(int32_t x, int32_t y)
{
    k3win32WinImpl* d = static_cast<k3win32WinImpl*>(_data);
    POINT p = { 0, 0 };
    ClientToScreen(d->_hwnd, &p);
    p.x += x;
    p.y += y;
    SetCursorPos(p.x, p.y);
}

K3API void k3winObj::SetVisible(bool visible)
{
    k3win32WinImpl* d = static_cast<k3win32WinImpl*>(_data);
    if (d->_is_visible == visible) return;

    d->_is_visible = visible;
    ShowWindow(d->_hwnd, ((visible) ? SW_SHOWDEFAULT : SW_HIDE));
}

K3API void k3winObj::SetCursorVisible(bool visible)
{
    k3win32WinImpl* d = static_cast<k3win32WinImpl*>(_data);
    d->_is_cursor_visible = visible;
    if (!d->_mouse_in_nc) d->SetWin32CursorState(d->_is_cursor_visible);
}

K3API void k3winObj::SetJoystickAttribute(uint32_t joystick, k3joyAttr attr_type, uint32_t num_values, float* values)
{
    uint32_t j;
    for (j = 0; j < k3win32WinImpl::_num_joy; j++) {
        if (k3win32WinImpl::_joy_map[j]->getDevId() == joystick) break;
    }
    if (j < k3win32WinImpl::_num_joy) {
        k3win32WinImpl::_joy_map[j]->SetAttribute(attr_type, num_values, values);
    } else {
        k3error::Handler("Joystick not found", "SetJoystickAttribute");
    }
}

K3API k3timer k3winObj::CreateTimer()
{
    k3timer timer = new k3timerObj();
    return timer;
}

K3API k3soundBuf k3winObj::CreateSoundBuffer(uint32_t num_channels, uint32_t samples_per_second, uint32_t bits_per_sample, uint32_t num_samples)
{
    k3soundBuf sbuf = new k3soundBufObj;
    if (k3soundBufImpl::_dsound) {
        k3win32WinImpl* d = static_cast<k3win32WinImpl*>(_data);
        k3soundBufImpl* sbuf_impl = sbuf->getImpl();

        // Set sound cooperative level priority_queue
        k3soundBufImpl::_dsound->SetCooperativeLevel(d->_hwnd, DSSCL_PRIORITY);

        HRESULT hr;
        WAVEFORMATEX wfx;
        DSBUFFERDESC dsbuf_desc;
        LPDIRECTSOUNDBUFFER buf;

        memset(&wfx, 0, sizeof(WAVEFORMATEX));
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = static_cast<WORD>(num_channels);
        wfx.nSamplesPerSec = samples_per_second;
        wfx.wBitsPerSample = static_cast<WORD>(bits_per_sample);
        wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        memset(&dsbuf_desc, 0, sizeof(DSBUFFERDESC));
        dsbuf_desc.dwSize = sizeof(DSBUFFERDESC);
        dsbuf_desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
        dsbuf_desc.dwBufferBytes = wfx.nBlockAlign * num_samples;
        dsbuf_desc.lpwfxFormat = &wfx;

        hr = k3soundBufImpl::_dsound->CreateSoundBuffer(&dsbuf_desc, &buf, NULL);
        if (SUCCEEDED(hr)) {
            buf->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*) &sbuf_impl->_buf);
            buf->Release();
        }
        sbuf_impl->_buf_size = dsbuf_desc.dwBufferBytes;
        return sbuf;
    }
    return NULL;
}


void k3winObj::WindowLoop()
{
    MSG msg;
    bool got_idle = false;
    bool got_joy = false;

    uint32_t w, j;
    HHOOK KeyboardLowLevelHook = SetWindowsHookEx(WH_KEYBOARD_LL, k3win32WinImpl::KeyboardLowLevelProc, GetModuleHandle(NULL), 0);
    HWND hwnd;
    k3win32WinImpl* winimpl;
    k3win32Joy joy;

    // Loops through all the windows, invalidating their client areas
    for (w = 0; w < k3win32WinImpl::_win_count; w++) {
        winimpl = k3win32WinImpl::_winimpl_map[w];
        hwnd = winimpl->_hwnd;

        if (winimpl->_is_visible) {
            ShowWindow(hwnd, SW_SHOWDEFAULT);
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }
        if (winimpl->Idle != NULL) got_idle = true;
    }

    ZeroMemory(&msg, sizeof(MSG));

    bool got_msg;
    got_joy = k3win32WinImpl::PollJoysticks();

    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    while (msg.message != WM_QUIT) {
        if (got_idle || got_joy) {
            got_msg = (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0);
        } else {
            GetMessage(&msg, NULL, 0, 0);
            got_msg = true;
        }
        if (got_msg) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        got_idle = false;
        for (w = 0; w < k3win32WinImpl::_win_count; w++) {
            winimpl = k3win32WinImpl::_winimpl_map[w];
            if (winimpl->Idle != NULL) {
                got_idle = true;
                // Allow idle when getting WM_INPUT messages; there can be lots of these
                if (!got_msg || msg.message == WM_INPUT) winimpl->Idle(winimpl->_data);
            }
        }
        got_joy = k3win32WinImpl::PollJoysticks();
        if (k3win32WinImpl::_win_count == 0) PostQuitMessage(0);
    }

    UnhookWindowsHookEx(KeyboardLowLevelHook);

    // Delete all joysticks
    for (j = 0; j < k3win32WinImpl::_num_joy; j++) {
        k3win32WinImpl::_joy_map[j] = NULL;
    }
    k3win32WinImpl::_num_joy = 0;

    // Delete all windows
    for (w = 0; w < k3win32WinImpl::_win_count; w++) {
        k3win32WinImpl::_win_map[w] = NULL;
    }
    k3win32WinImpl::_win_count = 0;
}

void k3winObj::ExitLoop()
{
    if (k3win32WinImpl::_win_count) {
        PostQuitMessage(0);
        if (k3win32WinImpl::_winimpl_map[0]->gfx != NULL) {
            k3win32WinImpl::_winimpl_map[0]->gfx->WaitGpuIdle();
        }
    }
    uint32_t w;
    for (w = 0; w < k3win32WinImpl::_win_count; w++) {
        k3win32WinImpl::_win_map[w] = NULL;
        k3win32WinImpl::_winimpl_map[w] = NULL;
    }
    k3win32WinImpl::_win_count = 0;
    k3win32WinImpl::Uninitialize();
}


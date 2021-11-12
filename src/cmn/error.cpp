// k3 graphics library
// functions to handle errors

#include "k3internal.h"

void K3CALLBACK k3error_NullHandler(const char*, const char*)
{ }

void K3CALLBACK k3error_StdOutHandler(const char* error_msg, const char* title)
{
    if (title) {
        printf("%s: %s\n", title, error_msg);
    } else {
        printf("%s\n", error_msg);
    }
}

k3error_handler_ptr k3error::_handler = k3error_StdOutHandler;

K3API void k3error::SetHandler(k3error_handler_ptr error_handler)
{
    if(error_handler) _handler = error_handler;
}

K3API k3error_handler_ptr k3error::GetHandler()
{
    return _handler;
}

K3API void k3error::Handler(const char* error, const char* title)
{
    _handler(error, title);
}

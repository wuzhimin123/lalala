#include <timeros/os.h>
#include <timeros/syscall.h>
static char out_buf[1000]; // buffer for _vprintf()
static int _vprintf(const char* s, va_list vl)
{
	int res = _vsnprintf(NULL, -1, s, vl);
	_vsnprintf(out_buf, res + 1, s, vl);
	sys_write(stdout,out_buf,res + 1);
	return res;
}

int printf(const char* s, ...)
{
	int res = 0;
	va_list vl;
	va_start(vl, s);
	res = _vprintf(s, vl);
	va_end(vl);
	return res;
}

#ifndef __STD__STDDEF_H__
#define __STD__STDDEF_H__

#define NULL ((void*)0)

#define offsetof(type, member) ((size_t) &((type*)0)->member)

typedef unsigned int size_t;
typedef int ptrdiff_t;
typedef unsigned int wchar_t;
typedef unsigned int wint_t;



#endif

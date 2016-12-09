#pragma once
#include <vector>
#include <stdint.h>

typedef uint8_t OpType;
typedef std::vector<OpType> OpPack;

enum ArgType
{
	Val,
	Ptr
};

#define null_arg		{ Val, 0 }

#define x86CALLEAX		0xff, 0xd0
#define x86CALLREF		0xff, 0x15
#define x86RET			0xc3

#define x86PUSHDIR8		0x6a
#define x86PUSHDIR32	0x68
#define x86PUSHREF32	0xff, 0x35

#define x86MOVEAXDIR	0xb8
#define x86MOVEAXREF	0xa1

#define x86BRK			0xcc
#include <windows.h>
#include <map>
#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <memory>

#include "Opcode.h"

struct FunctionType
{
	HMODULE ModuleHandle;
	HINSTANCE FunctionHandle;

	std::vector<OpType> AddressBytes;

	FunctionType(const std::string mod_name, const std::string func_name)
	{
		ModuleHandle = LoadLibraryA(mod_name.c_str());

		if (!ModuleHandle)
			throw std::runtime_error("Failed to import module");

		FunctionHandle = reinterpret_cast<HINSTANCE>(GetProcAddress(ModuleHandle, func_name.c_str()));

		if (!FunctionHandle)
			throw std::runtime_error("Failed to import function");

		AddressBytes.resize(sizeof(uint32_t));
		memcpy(&AddressBytes[0], &FunctionHandle, sizeof(uint32_t));
	}

	~FunctionType()
	{
		FreeLibrary(ModuleHandle);
		FunctionHandle = nullptr;
	}
};


struct VM
{
	typedef void(__stdcall*program_template)(void);

	OpPack bytecode;
	std::map<std::string, std::vector<uint8_t>> data_object;
	std::map<std::string, std::shared_ptr<FunctionType>> function_object;

	template <typename T>
	void AddData(const std::string name, const T data)
	{
		data_object[name] = { data.begin(), data.end() };
	}

	bool AddFunction(const std::string internal_name, const std::string module_name, const std::string func_name)
	{
		try
		{
			function_object[internal_name] = std::make_shared<FunctionType>(module_name, func_name);
		} catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			return false;
		}

		return true;
	}

	bool AddCall(std::string func_name, std::vector<std::pair<ArgType, uint32_t>> arg_pack)
	{
		if (function_object.find(func_name) == function_object.end())
			return false;

		std::reverse(arg_pack.begin(), arg_pack.end());

		for (const auto& arg : arg_pack)
		{
			switch (arg.first)
			{
			case Val:
				AddBytecode({ x86PUSHDIR8, 0x00 });
				break;

			case Ptr:
				break;

			default:
				AddBytecode({ x86PUSHDIR8, 0x00 });
				break;
			}
		}

		AddBytecode({ x86MOVEAXDIR });
		AddBytecode(function_object[func_name]->AddressBytes);
		AddBytecode({ x86CALLEAX });

		return true;
	}
	
	void AddBytecode(const OpPack ops)
	{
		bytecode.insert(bytecode.end(), ops.begin(), ops.end());
	}

	bool Exec(bool debug = false)
	{
		HANDLE func_pointer = VirtualAlloc(nullptr, bytecode.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		try
		{
			if (!func_pointer || bytecode.empty())
				throw std::runtime_error("Empty bytecode vector or null function pointer");

			memcpy(func_pointer, bytecode.data(), bytecode.size());
			
			if (debug)
				__debugbreak();
			
			reinterpret_cast<program_template>(func_pointer)();

		} catch (const std::exception& e)
		{
			VirtualFree(func_pointer, 0, MEM_RELEASE);

			std::cerr << e.what() << std::endl;
			return false;
		}

		return true;
	}

	static OpPack u32ToOp(uint32_t addr)
	{
		static OpPack ret_pack(4);

		memcpy(&ret_pack[0], &addr, sizeof(uint32_t));

		return ret_pack;
	}
};

static std::unique_ptr<VM> virtual_machine = nullptr;

int main(int argc, const char* argv[])
{
	try
	{
		virtual_machine = std::make_unique<VM>();
	} catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		std::cin.ignore();

		return -1;
	}

	virtual_machine->AddFunction("messagebox", "user32.dll", "MessageBoxA");

	virtual_machine->AddCall("messagebox", { { Val, 0 }, { Val, 0 }, { Val, 0 }, { Val, 0 } });

	virtual_machine->AddBytecode({ x86RET });

	if (!virtual_machine->Exec(false))
		std::cin.ignore();

	return 0;
}